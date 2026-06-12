#include "Bits_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
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

Bits_Block::Bits_Block(const std::string& name)
    : Block(name)
    , m_probOfZero(0.5)
    , m_bitRate(0.0)
    , m_showAdvancedParams(Bits::NO)
    , m_sampleRateOption(Bits::TimedfromSchematic)
    , m_sampleRate(0.0)
    , m_initialDelay(0)
    , m_burstMode(Bits::OFF)
    , m_burstLength(100)
    , m_burstPeriod(200)
    , m_burstDelay(0)
    , m_previousBitValue(false)
{
}

void Bits_Block::SetDefaultParamters()
{
    m_probOfZero = 0.5;
    m_bitRate = 0.0;
    m_showAdvancedParams = Bits::NO;
    m_sampleRateOption = Bits::TimedfromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
    m_burstMode = Bits::OFF;
    m_burstLength = 100;
    m_burstPeriod = 200;
    m_burstDelay = 0;
    m_previousBitValue = false;
}

void Bits_Block::SetParameters()
{
    if (!m_bits) {
        return;
    }

    m_bits->ProbOfZero = m_probOfZero;
    m_bits->BitRate = m_bitRate;
    m_bits->ShowAdvancedParams = m_showAdvancedParams;
    m_bits->SampleRateOption = m_sampleRateOption;
    m_bits->SampleRate = m_sampleRate;
    m_bits->InitialDelay = m_initialDelay;
    m_bits->BurstMode = m_burstMode;
    m_bits->BurstLength = m_burstLength;
    m_bits->BurstPeriod = m_burstPeriod;
    m_bits->BurstDelay = m_burstDelay;
    m_bits->previousBitValue = m_previousBitValue;
}

bool Bits_Block::Setup()
{
    Block::Setup();
    qDebug() << "bit setup";
    return true;
}

bool Bits_Block::Run()
{
    if (!m_bits) {
        return false;
    }

    if (!m_bits->Run()) {
        return false;
    }

    std::vector<int> outputData;
    outputData.push_back(m_bits->output[0U]);
    WriteOutputData(GetOutputPortName(0), outputData);

    m_bits->Advance();
    return true;
}

bool Bits_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_bits = std::make_unique<Bits>();

    AddOutputPort("output", m_bits->output, 1, Block::DataType::TIMED_INT);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_probOfZero = std::stod(getParameter("ProbOfZero").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ProbOfZero', using default value."); }
    try { m_bitRate = std::stod(getParameter("BitRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BitRate', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvanced(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstMode', using default value."); }
    try { m_burstLength = std::stoi(getParameter("BurstLength").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstLength', using default value."); }
    try { m_burstPeriod = std::stoi(getParameter("BurstPeriod").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstPeriod', using default value."); }
    try { m_burstDelay = std::stoi(getParameter("BurstDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstDelay', using default value."); }

    if (m_probOfZero < 0.0 || m_probOfZero > 1.0) {
        std::cout << "Bits_self: ProbOfZero must be between 0 and 1." << std::endl;
        return false;
    }

    double sampleRate = m_sampleRate;
    if (m_sampleRateOption == Bits::TimedfromSchematic) {
        sampleRate = simulator_param.samplingRate;
    }

    if (m_sampleRateOption == Bits::TimedfromSampleRate && m_sampleRate <= 0.0) {
        std::cout << "Bits_self: SampleRate must be > 0 when timed from SampleRate." << std::endl;
        return false;
    }

    if (m_bitRate < 0.0 || (sampleRate > 0.0 && m_bitRate > sampleRate)) {
        std::cout << "Bits_self: BitRate must be >= 0 and <= SampleRate." << std::endl;
        return false;
    }

    if (m_initialDelay < 0) {
        std::cout << "Bits_self: InitialDelay must be >= 0." << std::endl;
        return false;
    }

    if (m_burstLength < 1) {
        std::cout << "Bits_self: BurstLength must be >= 1." << std::endl;
        return false;
    }

    if (m_burstPeriod < 1) {
        std::cout << "Bits_self: BurstPeriod must be >= 1." << std::endl;
        return false;
    }

    if (m_burstMode == Bits::Multiple &&
        (m_burstDelay < 0 || m_burstDelay > m_burstPeriod - m_burstLength)) {
        std::cout << "Bits_self: BurstDelay must be >= 0 and <= (BurstPeriod - BurstLength)."
                  << std::endl;
        return false;
    }

    SetParameters();

    return m_bits->Setup();
}

Bits::ShowAdvancedParamsEnum Bits_Block::ConvertStringToShowAdvanced(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Bits::NO;
    }
    if (lower == "yes" || lower == "1") return Bits::YES;
    return Bits::NO;
}

Bits::SampleRateOptionEnum Bits_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") return Bits::UnTimed;
    if (lower == "timedfromsamplerate" || lower == "1") return Bits::TimedfromSampleRate;
    if (lower == "timed from samplerate") return Bits::TimedfromSampleRate;
    if (lower == "timedfromschematic" || lower == "2") return Bits::TimedfromSchematic;
    if (lower == "timed from schematic") return Bits::TimedfromSchematic;
    return Bits::TimedfromSchematic;
}

Bits::BurstModeEnum Bits_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return Bits::OFF;
    }
    if (lower == "single" || lower == "1") return Bits::Single;
    if (lower == "multiple" || lower == "2") return Bits::Multiple;
    return Bits::OFF;
}




