#include "ComplexExpGen_Block.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
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

ComplexExpGen_Block::ComplexExpGen_Block(const std::string& name)
    : Block(name)
{
}

void ComplexExpGen_Block::SetDefaultParamters()
{
    m_amplitude = 1.0;
    m_offset = std::complex<double>(0.0, 0.0);
    m_frequency = 5e3;
    m_phase = 0.0;
    m_quadraturePolarity = ComplexExpGen::normal;
    m_showAdvancedParams = ComplexExpGen::No;
    m_sampleRateOption = ComplexExpGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
    m_burstMode = ComplexExpGen::OFF;
    m_burstLength = 100e-6;
    m_burstPeriod = 200e-6;
    m_burstDelay = 0.0;
}

void ComplexExpGen_Block::SetParameters()
{
    if (!m_complexExpGen) {
        return;
    }

    m_complexExpGen->Amplitude = m_amplitude;
    m_complexExpGen->Offset = m_offset;
    m_complexExpGen->Frequency = m_frequency;
    m_complexExpGen->Phase = m_phase;
    m_complexExpGen->QuadraturePolarity = m_quadraturePolarity;
    m_complexExpGen->ShowAdvancedParams = m_showAdvancedParams;
    m_complexExpGen->SampleRateOption = m_sampleRateOption;
    m_complexExpGen->SampleRate = m_sampleRate;
    m_complexExpGen->InitialDelay = m_initialDelay;
    m_complexExpGen->BurstMode = m_burstMode;
    m_complexExpGen->BurstLength = m_burstLength;
    m_complexExpGen->BurstPeriod = m_burstPeriod;
    m_complexExpGen->BurstDelay = m_burstDelay;

}

bool ComplexExpGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool ComplexExpGen_Block::Run()
{
    const double sampleRate = (m_sampleRateOption == ComplexExpGen::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;
    
    if (sampleRate <= 0.0) {
        return false;
    }

    if (m_frequency < -sampleRate / 2.0 || m_frequency > sampleRate / 2.0) {
        return false;
    }

    if (m_burstLength < 1.0 / sampleRate) {
        return false;
    }

    if (m_burstPeriod < 1.0 / sampleRate) {
        return false;
    }

    const double PI = std::acos(-1.0);
    const double t = simulator_param.startTime
        + static_cast<double>(m_complexExpGen->GetCount()) / sampleRate
        + 1e-16;
    const double polaritySign = (m_quadraturePolarity == ComplexExpGen::inverted) ? -1.0 : 1.0;

    std::complex<double> y(0.0, 0.0);
    switch (m_burstMode) {
    case ComplexExpGen::OFF:
        if (t >= m_initialDelay) {
            y = m_amplitude * std::exp(std::complex<double>(0.0, 2.0 * polaritySign * PI * m_frequency * (t - m_initialDelay) + m_phase)) + m_offset;
        } else {
            y = 0.0;
        }
        break;
    case ComplexExpGen::Single:
        if (t >= m_initialDelay && t < m_initialDelay + m_burstDelay) {
            y = m_offset;
        } else if (t >= m_initialDelay + m_burstDelay && t < m_initialDelay + m_burstDelay + m_burstLength) {
            y = m_amplitude * std::exp(std::complex<double>(0.0, 2.0 * polaritySign * PI * m_frequency * (t - m_initialDelay - m_burstDelay) + m_phase)) + m_offset;
        } else {
            y = 0.0;
        }
        break;
    case ComplexExpGen::Multiple: {
        const double wt = std::fmod(t - m_initialDelay, m_burstPeriod);
        if (wt >= m_burstDelay && wt < m_burstDelay + m_burstLength) {
            y = m_amplitude * std::exp(std::complex<double>(0.0, 2.0 * polaritySign * PI * m_frequency * (wt - m_burstDelay) + m_phase)) + m_offset;
        } else {
            y = m_offset;
        }
        break;
    }
    default:
        break;
    }

    if (t < m_initialDelay) {
        y = 0.0;
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);

    m_complexExpGen->Advance();
    return true;
}

bool ComplexExpGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_complexExpGen = std::make_unique<ComplexExpGen>();

    AddOutputPort("output", m_complexExpGen->output, 1, Block::DataType::TIMED_DCOMPLEX);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_amplitude = std::stod(getParameter("Amplitude").Value); } catch (...) { }
    try { m_offset = ParseComplex(getParameter("Offset").Value); } catch (...) { }
    try { m_frequency = std::stod(getParameter("Frequency").Value); } catch (...) { }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { }
    try { m_quadraturePolarity = ConvertStringToQuadraturePolarity(getParameter("QuadraturePolarity").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { }
    try { m_burstLength = std::stod(getParameter("BurstLength").Value); } catch (...) { }
    try { m_burstPeriod = std::stod(getParameter("BurstPeriod").Value); } catch (...) { }
    try { m_burstDelay = std::stod(getParameter("BurstDelay").Value); } catch (...) { }

    SetParameters();

    const double outFs = (m_sampleRateOption == ComplexExpGen::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_complexExpGen->output.SetSampleRate(outFs);

    if (!m_complexExpGen->Setup()) {
        return false;
    }

    return true;
}

ComplexExpGen::SelectedQuadraturePolarity ComplexExpGen_Block::ConvertStringToQuadraturePolarity(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "normal") {
        return ComplexExpGen::normal;
    }
    if (lower == "inverted" || lower == "1") {
        return ComplexExpGen::inverted;
    }
    return ComplexExpGen::normal;
}

ComplexExpGen::SelectedShowAdvancedParams ComplexExpGen_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return ComplexExpGen::No;
    }
    if (lower == "yes" || lower == "1") {
        return ComplexExpGen::Yes;
    }
    return ComplexExpGen::No;
}

ComplexExpGen::SelectedSampleRateOption ComplexExpGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return ComplexExpGen::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return ComplexExpGen::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return ComplexExpGen::TimedFromSchematic;
    }
    return ComplexExpGen::TimedFromSchematic;
}

ComplexExpGen::SelectedBurstMode ComplexExpGen_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return ComplexExpGen::OFF;
    }
    if (lower == "single" || lower == "1") {
        return ComplexExpGen::Single;
    }
    if (lower == "multiple" || lower == "2") {
        return ComplexExpGen::Multiple;
    }
    return ComplexExpGen::OFF;
}

std::complex<double> ComplexExpGen_Block::ParseComplex(const std::string& value)
{
    std::string s = TrimCopy(value);
    if (s.empty()) {
        return std::complex<double>(0.0, 0.0);
    }

    std::string t = s;
    for (char& ch : t) {
        if (ch == 'i' || ch == 'I' || ch == 'j' || ch == 'J') {
            ch = ' ';
        }
    }

    size_t splitPos = t.find(',');
    if (splitPos == std::string::npos) {
        splitPos = t.find(';');
    }

    if (splitPos != std::string::npos) {
        double re = 0.0;
        double im = 0.0;
        try { re = std::stod(TrimCopy(t.substr(0, splitPos))); } catch (...) { re = 0.0; }
        try { im = std::stod(TrimCopy(t.substr(splitPos + 1))); } catch (...) { im = 0.0; }
        return std::complex<double>(re, im);
    }

    size_t pos = std::string::npos;
    for (size_t i = 1; i < t.size(); ++i) {
        if (t[i] == '+' || t[i] == '-') {
            pos = i;
        }
    }

    if (pos != std::string::npos) {
        double re = 0.0;
        double im = 0.0;
        try { re = std::stod(TrimCopy(t.substr(0, pos))); } catch (...) { re = 0.0; }
        try { im = std::stod(TrimCopy(t.substr(pos))); } catch (...) { im = 0.0; }
        return std::complex<double>(re, im);
    }

    double re = 0.0;
    try { re = std::stod(TrimCopy(t)); } catch (...) { re = 0.0; }
    return std::complex<double>(re, 0.0);
}








