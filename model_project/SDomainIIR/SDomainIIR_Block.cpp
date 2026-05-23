#include "SDomainIIR_Block.h"

#include <algorithm>
#include <cctype>
#include <complex>
#include <sstream>

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

std::vector<double> ParseVectorDouble(const std::string& value)
{
    std::vector<double> result;

    std::string trimmed = TrimCopy(value);
    if(trimmed == "[]" || trimmed.empty()) {
        return result;
    }

    try {
        auto mat = DataTypesAndParsers::ParseStringToMatrixDouble(value);
        result.reserve(mat.NumElements());
        for (size_t i = 0; i < mat.NumElements(); ++i) {
            result.push_back(mat(i));
        }
        return result;
    } catch (...) {
        std::string s = value;
                s.erase(std::remove(s.begin(), s.end(), '['), s.end());
                s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
                std::replace(s.begin(), s.end(), ',', ' ');

                std::stringstream ss(s);
                std::string token;
                while (ss >> token) {
                    try {
                        double val = std::stod(token);
                        result.push_back(val);
                    } catch (...) {
                        LOG_ERROR("解析数值失败:",token);
                    }
                }
    }

    return result;
}

std::vector<std::complex<double>> ParseVectorDComplex(const std::string& value)
{
    std::vector<std::complex<double>> result;

    try {
        auto mat = DataTypesAndParsers::ParseStringToMatrixDComplex(value);
        result.reserve(mat.NumElements());
        for (size_t i = 0; i < mat.NumElements(); ++i) {
            result.push_back(mat(i));
        }
        return result;
    } catch (...) {
    }
    return result;
}
}

SDomainIIR_Block::SDomainIIR_Block(const std::string& name)
    : Block(name)
{
}

void SDomainIIR_Block::SetDefaultParamters()
{
    m_sampleRate = 1.0e6;
    m_factor = 1.1616128054708951e+029;
    m_realPoles = { -650148.07080641726 };
    m_complexPoles = {
        std::complex<double>(-200906.80273926951, 618327.55929716607),
        std::complex<double>(-525980.83814247814, 382147.44782641466)
    };
    m_realZeros.clear();
    m_complexZeros.clear();
    m_freqUnit = SDomainIIR::FREQ_RAD_PER_SEC;
}

void SDomainIIR_Block::SetParameters()
{
    if (!m_sdomainIIR) {
        return;
    }

    m_sdomainIIR->SampleRate = m_sampleRate;
    m_sdomainIIR->Factor = m_factor;
    m_sdomainIIR->FreqUnit = m_freqUnit;

    m_sdomainIIR->RealPoles = m_realPoles.empty() ? nullptr : m_realPoles.data();
    m_sdomainIIR->RealPolesSize = static_cast<int>(m_realPoles.size());

    m_sdomainIIR->ComplexConjugatePoles = m_complexPoles.empty() ? nullptr : m_complexPoles.data();
    m_sdomainIIR->ComplexConjugatePolesSize = static_cast<int>(m_complexPoles.size());

    m_sdomainIIR->RealZeros = m_realZeros.empty() ? nullptr : m_realZeros.data();
    m_sdomainIIR->RealZerosSize = static_cast<int>(m_realZeros.size());

    m_sdomainIIR->ComplexConjugateZeros = m_complexZeros.empty() ? nullptr : m_complexZeros.data();
    m_sdomainIIR->ComplexConjugateZerosSize = static_cast<int>(m_complexZeros.size());
}

bool SDomainIIR_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SDomainIIR_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_sdomainIIR) {
        return false;
    }

    const std::string inputPortName = GetInputPortName(0);
    const std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    const auto& a = m_sdomainIIR->GetA();
    const auto& b = m_sdomainIIR->GetB();
    auto& state = m_sdomainIIR->GetState();

    const int Na = static_cast<int>(a.size());
    const int Nb = static_cast<int>(b.size());
    const int N = static_cast<int>(state.size());

    std::vector<double> outputData;
    outputData.reserve(inputData.size());

    for (double x : inputData) {
        if (a.empty() || b.empty()) {
            outputData.push_back(x);
            continue;
        }

        double y = 0.0;

        if (N == 0) {
            const double b0 = (Nb > 0) ? b[0] : 0.0;
            y = b0 * x;
        } else {
            double* s = state.data();

            const double acc = (Nb > 0 ? b[0] * x : 0.0) + s[0];
            y = acc;

            for (int i = 0; i < N - 1; ++i) {
                double next = s[i + 1];

                if (i + 1 < Nb) {
                    next += b[i + 1] * x;
                }

                if (i + 1 < Na) {
                    next -= a[i + 1] * y;
                }

                s[i] = next;
            }

            double last = 0.0;
            if (N < Nb) {
                last += b[N] * x;
            }
            if (N < Na) {
                last -= a[N] * y;
            }

            s[N - 1] = last;
        }

        outputData.push_back(y);
    }

    WriteOutputData(outputPortName, outputData);

    return true;
}

bool SDomainIIR_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_sdomainIIR = std::make_unique<SystemVueModelBuilder::SDomainIIR>();

    AddInputPort("input", m_sdomainIIR->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_sdomainIIR->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    try { m_factor = std::stod(getParameter("Factor").Value); } catch (...) { }
    try { m_realPoles = ParseVectorDouble(getParameter("RealPoles").Value); } catch (...) { }
    try { m_complexPoles = ParseVectorDComplex(getParameter("ComplexConjugatePoles").Value); } catch (...) { }
    try { m_realZeros = ParseVectorDouble(getParameter("RealZeros").Value); } catch (...) { }
    try { m_freqUnit = ConvertStringToFreqUnit(getParameter("FreqUnit").Value); } catch (...) { }
    SetParameters();

    return m_sdomainIIR->Initialize();
}

SDomainIIR::FreqUnitEnum SDomainIIR_Block::ConvertStringToFreqUnit(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "freq_hz" || lower == "hz" || lower == "1") {
        return SDomainIIR::FREQ_HZ;
    }
    if (lower == "freq_rad_per_sec" || lower == "radians per second" || lower == "radians per sec" || lower == "0") {
        return SDomainIIR::FREQ_RAD_PER_SEC;
    }
    return SDomainIIR::FREQ_RAD_PER_SEC;
}
