#include "TimeDelayInt_Block.h"
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

TimeDelayInt_Block::TimeDelayInt_Block(const std::string& name)
    : Block(name)
{
}

void TimeDelayInt_Block::SetDefaultParamters()
{
    m_unit = TimeDelayInt::Unit_Time;
    m_T = 0.0;
    m_N = 0;
    m_delaySeconds = 0.0;
    m_latencyReady = false;
}

void TimeDelayInt_Block::SetParameters()
{
    if (!m_timeDelayInt) {
        return;
    }

    m_timeDelayInt->Unit = m_unit;
    m_timeDelayInt->T = m_T;
    m_timeDelayInt->N = m_N;
}

bool TimeDelayInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TimeDelayInt_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    if (!m_latencyReady) {
        UpdateLatency();
    }

    std::vector<int> outputData(inputData.begin(), inputData.end());
    WriteOutputData(outputPort, outputData);

    if (m_timeDelayInt) {
        m_timeDelayInt->Advance();
    }

    return true;
}

bool TimeDelayInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_timeDelayInt = std::make_unique<TimeDelayInt>();

    AddInputPort("input", m_timeDelayInt->input, 1, Block::DataType::TIMED_INT);
    AddOutputPort("output", m_timeDelayInt->output, 1, Block::DataType::TIMED_INT);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_unit = ConvertStringToUnitEnum(getParameter("Unit").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Unit', using default value."); }
    try { m_T = std::stod(getParameter("T").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'T', using default value."); }
    try { m_N = std::stoi(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }

    if (m_T < 0.0) {
        std::cout << "TimeDelayInt: T must be >= 0." << std::endl;
        return false;
    }
    if (m_N < 0) {
        std::cout << "TimeDelayInt: N must be >= 0." << std::endl;
        return false;
    }

    SetParameters();

    return true;
}

bool TimeDelayInt_Block::UpdateLatency()
{
    if (!m_timeDelayInt) {
        return false;
    }

    const double fsIn = simulator_param.samplingRate;
    const double dtIn = simulator_param.time_Interval;
    const double t0 = simulator_param.startTime;

    if (m_unit == TimeDelayInt::Unit_Time) {
        m_delaySeconds = m_T;
    } else {
        if (dtIn <= 0.0) {
            std::cout << "TimeDelayInt: input signal must be timed (sample rate > 0)." << std::endl;
            return false;
        }
        m_delaySeconds = static_cast<double>(m_N) * dtIn;
    }

    if (fsIn > 0.0) {
        m_timeDelayInt->output.SetSampleRate(fsIn);
        m_timeDelayInt->output.SetStartTime(t0 + m_delaySeconds);
    }

    m_latencyReady = true;
    return true;
}

TimeDelayInt::UnitEnum TimeDelayInt_Block::ConvertStringToUnitEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "unit_timestep") {
        return TimeDelayInt::Unit_TimeStep;
    }
    if (lower == "unit_time") {
        return TimeDelayInt::Unit_Time;
    }
    if (lower == "timestep" || lower == "1") {
        return TimeDelayInt::Unit_TimeStep;
    }
    return TimeDelayInt::Unit_Time;
}
