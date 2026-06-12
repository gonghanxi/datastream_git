#include "TimeDelay_Block.h"
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

TimeDelay_Block::TimeDelay_Block(const std::string& name)
    : Block(name)
{
}

void TimeDelay_Block::SetDefaultParamters()
{
    m_unit = TimeDelay::Unit_Time;
    m_T = 0.0;
    m_N = 0;
    m_delaySeconds = 0.0;
    m_latencyReady = false;
}

void TimeDelay_Block::SetParameters()
{
    if (!m_timeDelay) {
        return;
    }

    m_timeDelay->Unit = m_unit;
    m_timeDelay->T = m_T;
    m_timeDelay->N = m_N;
}

bool TimeDelay_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TimeDelay_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    if (!m_latencyReady) {
        UpdateLatency();
    }

    std::vector<double> outputData(inputData.begin(), inputData.end());
    WriteOutputData(outputPort, outputData);

    if (m_timeDelay) {
        m_timeDelay->Advance();
    }

    return true;
}

bool TimeDelay_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_timeDelay = std::make_unique<TimeDelay>();

    AddInputPort("input", m_timeDelay->input, 1, Block::DataType::TIMED_DOUBLE);
    AddOutputPort("output", m_timeDelay->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_unit = ConvertStringToUnitEnum(getParameter("Unit").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Unit', using default value."); }
    try { m_T = std::stod(getParameter("T").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'T', using default value."); }
    try { m_N = std::stoi(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }

    if (m_T < 0.0) {
        std::cout << "TimeDelay: T must be >= 0." << std::endl;
        return false;
    }
    if (m_N < 0) {
        std::cout << "TimeDelay: N must be >= 0." << std::endl;
        return false;
    }

    SetParameters();

    return true;
}

bool TimeDelay_Block::UpdateLatency()
{
    if (!m_timeDelay) {
        return false;
    }

    const double fsIn = simulator_param.samplingRate;
    const double dtIn = simulator_param.time_Interval;
    const double t0 = simulator_param.startTime;

    std::cout << "TimeDelay: using simu params"
              << " samplingRate=" << fsIn
              << " time_Interval=" << dtIn
              << " startTime=" << t0
              << std::endl;

    if (m_unit == TimeDelay::Unit_Time) {
        m_delaySeconds = m_T;
    } else {
        if (dtIn <= 0.0) {
            std::cout << "TimeDelay: input signal must be timed (sample rate > 0)." << std::endl;
            return false;
        }
        m_delaySeconds = static_cast<double>(m_N) * dtIn;
    }

    if (fsIn > 0.0) {
        m_timeDelay->output.SetSampleRate(fsIn);
        m_timeDelay->output.SetStartTime(t0 + m_delaySeconds);
    }

    m_latencyReady = true;
    return true;
}

TimeDelay::UnitEnum TimeDelay_Block::ConvertStringToUnitEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "unit_timestep") {
        return TimeDelay::Unit_TimeStep;
    }
    if (lower == "unit_time") {
        return TimeDelay::Unit_Time;
    }
    if (lower == "timestep" || lower == "1") {
        return TimeDelay::Unit_TimeStep;
    }
    return TimeDelay::Unit_Time;
}
