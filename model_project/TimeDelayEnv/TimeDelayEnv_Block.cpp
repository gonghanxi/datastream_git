#include "TimeDelayEnv_Block.h"
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

TimeDelayEnv_Block::TimeDelayEnv_Block(const std::string& name)
    : Block(name)
{
}

void TimeDelayEnv_Block::SetDefaultParamters()
{
    m_unit = TimeDelayEnv::Unit_Time;
    m_T = 0.0;
    m_N = 0;
    m_delaySeconds = 0.0;
    m_latencyReady = false;
}

void TimeDelayEnv_Block::SetParameters()
{
    if (!m_timeDelayEnv) {
        return;
    }

    m_timeDelayEnv->Unit = m_unit;
    m_timeDelayEnv->T = m_T;
    m_timeDelayEnv->N = m_N;
}

bool TimeDelayEnv_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TimeDelayEnv_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    if (!m_latencyReady) {
        UpdateLatency();
    }

    UpdateCharacterizationFrequency();

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData(inputData.begin(), inputData.end());
    WriteOutputData(outputPort, outputData);

    if (m_timeDelayEnv) {
        m_timeDelayEnv->Advance();
    }

    return true;
}

bool TimeDelayEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_timeDelayEnv = std::make_unique<TimeDelayEnv>();

    AddInputPort("input", m_timeDelayEnv->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_timeDelayEnv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_unit = ConvertStringToUnitEnum(getParameter("Unit").Value); } catch (...) { }
    try { m_T = std::stod(getParameter("T").Value); } catch (...) { }
    try { m_N = std::stoi(getParameter("N").Value); } catch (...) { }

    if (m_T < 0.0) {
        std::cout << "TimeDelayEnv: T must be >= 0." << std::endl;
        return false;
    }
    if (m_N < 0) {
        std::cout << "TimeDelayEnv: N must be >= 0." << std::endl;
        return false;
    }

    SetParameters();

    return true;
}

bool TimeDelayEnv_Block::UpdateLatency()
{
    if (!m_timeDelayEnv) {
        return false;
    }

    const double fsIn = simulator_param.samplingRate;
    const double dtIn = simulator_param.time_Interval;
    const double t0 = simulator_param.startTime;

    if (m_unit == TimeDelayEnv::Unit_Time) {
        m_delaySeconds = m_T;
    } else {
        if (dtIn <= 0.0) {
            std::cout << "TimeDelayEnv: input signal must be timed (sample rate > 0)." << std::endl;
            return false;
        }
        m_delaySeconds = static_cast<double>(m_N) * dtIn;
    }

    if (fsIn > 0.0) {
        m_timeDelayEnv->output.SetSampleRate(fsIn);
        m_timeDelayEnv->output.SetStartTime(t0 + m_delaySeconds);
    }

    m_latencyReady = true;
    return true;
}

void TimeDelayEnv_Block::UpdateCharacterizationFrequency()
{
    auto* inPort = GetInputPort(GetInputPortName(0));
    auto* outPort = GetOutputPort(GetOutputPortName(0));
    if (!inPort || !outPort) {
        return;
    }

    const double fcIn = inPort->getCharacterizationFrequency();
    outPort->setCharacterizationFrequency(fcIn);
}

TimeDelayEnv::UnitEnum TimeDelayEnv_Block::ConvertStringToUnitEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "unit_timestep") {
        return TimeDelayEnv::Unit_TimeStep;
    }
    if (lower == "unit_time") {
        return TimeDelayEnv::Unit_Time;
    }
    if (lower == "timestep" || lower == "1") {
        return TimeDelayEnv::Unit_TimeStep;
    }
    return TimeDelayEnv::Unit_Time;
}


