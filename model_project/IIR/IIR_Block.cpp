#include "IIR_Block.h"
#include "DataTypesAndParsers.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cctype>
#include <sstream>

namespace {
std::vector<double> ParseVectorDouble(const std::string& value)
{
    std::vector<double> result;

    try {
        auto mat = DataTypesAndParsers::ParseStringToMatrixDouble(value);
        result.reserve(mat.NumElements());
        for (size_t i = 0; i < mat.NumElements(); ++i) {
            result.push_back(mat(i));
        }
        return result;
    } catch (...) {
        // Fall through to basic parse
    }

    std::string s = value;
    s.erase(std::remove(s.begin(), s.end(), '['), s.end());
    s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
    std::replace(s.begin(), s.end(), ',', ' ');

    std::stringstream ss(s);
    std::string token;
    while (ss >> token) {
        try {
            result.push_back(std::stod(token));
        } catch (...) {
        }
    }

    return result;
}
}

IIR_Block::IIR_Block(const std::string& name)
    : Block(name)
    , m_gain(1.0)
{
}

void IIR_Block::SetDefaultParamters()
{
    m_gain = 1.0;
    m_numerator = {0.5, 0.25, 0.1};
    m_denominator = {1.0, 0.5, 0.3};
}

void IIR_Block::ResetState()
{
    m_numState = static_cast<int>(std::max(m_numerator.size(), m_denominator.size())) - 1;
    if (m_numState < 0) {
        m_numState = 0;
    }

    m_state.assign(static_cast<size_t>(m_numState), 0.0);
}

void IIR_Block::SetParameters()
{
    if (!m_iir) {
        return;
    }

    m_iir->m_Gain = m_gain;
    m_iir->m_Numerator = m_numerator.empty() ? nullptr : m_numerator.data();
    m_iir->m_iNumeratorSize = static_cast<int>(m_numerator.size());
    m_iir->m_Denominator = m_denominator.empty() ? nullptr : m_denominator.data();
    m_iir->m_iDenominatorSize = static_cast<int>(m_denominator.size());

    ResetState();
}

bool IIR_Block::Setup()
{
    Block::Setup();
    return true;
}

bool IIR_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_iir = std::make_unique<IIR>();

    AddInputPort("input", m_iir->m_input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_iir->m_output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

    try { m_gain = std::stod(getParameter("Gain").Value); } catch (...) { }
    try { m_numerator = ParseVectorDouble(getParameter("Numerator").Value); } catch (...) { }
    try { m_denominator = ParseVectorDouble(getParameter("Denominator").Value); } catch (...) { }

    SetParameters();

    if (m_denominator.empty()) {
        std::cout << "IIR: Denominator coefficients are not specified." << std::endl;
        return false;
    }

    if (std::fabs(m_denominator[0]) < DBL_EPSILON) {
        std::cout << "IIR: Denominator[0] must be non-zero." << std::endl;
        return false;
    }

    if (m_numerator.empty()) {
        std::cout << "IIR: Numerator is empty, filter output will be zero." << std::endl;
    }

    return true;
}

bool IIR_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_iir) {
        return false;
    }

    const std::string inputPortName = GetInputPortName(0);
    const std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    std::vector<double> outputData;
    outputData.reserve(inputData.size());

    const int Nb = m_iir->m_iNumeratorSize;
    const int Na = m_iir->m_iDenominatorSize;

    if (!m_iir->m_Denominator || Na <= 0) {
        outputData.assign(inputData.size(), 0.0);
        WriteOutputData(outputPortName, outputData);
        return false;
    }

    const double a0 = m_iir->m_Denominator[0];
    if (std::fabs(a0) < DBL_EPSILON) {
        outputData.assign(inputData.size(), 0.0);
        WriteOutputData(outputPortName, outputData);
        return false;
    }

    for (double x : inputData) {
        double y = 0.0;

        if (m_numState == 0) {
            double acc = 0.0;
            if (Nb > 0) {
                acc = m_iir->m_Numerator[0] * x;
            }
            y = (m_iir->m_Gain * acc) / a0;
        } else {
            const int N = m_numState;

            double acc = (Nb > 0 ? m_iir->m_Numerator[0] * x : 0.0) + m_state[0];
            y = (m_iir->m_Gain * acc) / a0;

            for (int i = 0; i < N - 1; ++i) {
                double next = m_state[i + 1];

                if (i + 1 < Nb) {
                    next += m_iir->m_Numerator[i + 1] * x;
                }

                if (i + 1 < Na) {
                    next -= m_iir->m_Denominator[i + 1] * y;
                }

                m_state[i] = next;
            }

            double last = 0.0;
            if (N < Nb) {
                last += m_iir->m_Numerator[N] * x;
            }
            if (N < Na) {
                last -= m_iir->m_Denominator[N] * y;
            }

            m_state[static_cast<size_t>(N - 1)] = last;
        }

        outputData.push_back(y);
    }

    WriteOutputData(outputPortName, outputData);
    return true;
}
