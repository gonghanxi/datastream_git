#include "TimeDelayCx_Block.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

TimeDelayCx_Block::TimeDelayCx_Block(const std::string& name)
    : Block(name)
{
}

void TimeDelayCx_Block::SetDefaultParamters()
{
    m_unit = TimeDelayCx::Unit_Time;
    m_T = 0.0;
    m_N = 0;
    m_delaySeconds = 0.0;
    m_latencyReady = false;
}

void TimeDelayCx_Block::SetParameters()
{
    if (!m_timeDelayCx) {
        return;
    }

    m_timeDelayCx->Unit = m_unit;
    m_timeDelayCx->T = m_T;
    m_timeDelayCx->N = m_N;
}

bool TimeDelayCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TimeDelayCx_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    if (!m_latencyReady) {
        UpdateLatency();
    }

    std::vector<std::complex<double>> outputData(inputData.begin(), inputData.end());
    WriteOutputData(outputPort, outputData);

    if (m_timeDelayCx) {
        m_timeDelayCx->Advance();
    }

    return true;
}

bool TimeDelayCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_timeDelayCx = std::make_unique<TimeDelayCx>();

    AddInputPort("input", m_timeDelayCx->input, 1, Block::DataType::TIMED_DCOMPLEX);
    AddOutputPort("output", m_timeDelayCx->output, 1, Block::DataType::TIMED_DCOMPLEX);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_unit = ConvertStringToUnitEnum(getParameter("Unit").Value); } catch (...) { }
    try { m_T = std::stod(getParameter("T").Value); } catch (...) { }
    try { m_N = std::stoi(getParameter("N").Value); } catch (...) { }

    if (m_T < 0.0) {
        std::cout << "TimeDelayCx: T must be >= 0." << std::endl;
        return false;
    }
    if (m_N < 0) {
        std::cout << "TimeDelayCx: N must be >= 0." << std::endl;
        return false;
    }

    SetParameters();

    return true;
}

bool TimeDelayCx_Block::UpdateLatency()
{
    if (!m_timeDelayCx) {
        return false;
    }

    const double fsIn = simulator_param.samplingRate;
    const double dtIn = simulator_param.time_Interval;
    const double t0 = simulator_param.startTime;

    if (m_unit == TimeDelayCx::Unit_Time) {
        m_delaySeconds = m_T;
    } else {
        if (dtIn <= 0.0) {
            std::cout << "TimeDelayCx: input signal must be timed (sample rate > 0)." << std::endl;
            return false;
        }
        m_delaySeconds = static_cast<double>(m_N) * dtIn;
    }

    if (fsIn > 0.0) {
        m_timeDelayCx->output.SetSampleRate(fsIn);
        m_timeDelayCx->output.SetStartTime(t0 + m_delaySeconds);
    }

    m_latencyReady = true;
    return true;
}

TimeDelayCx::UnitEnum TimeDelayCx_Block::ConvertStringToUnitEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "unit_timestep") {
        return TimeDelayCx::Unit_TimeStep;
    }
    if (lower == "unit_time") {
        return TimeDelayCx::Unit_Time;
    }
    if (lower == "timestep" || lower == "1") {
        return TimeDelayCx::Unit_TimeStep;
    }
    return TimeDelayCx::Unit_Time;
}
