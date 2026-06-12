#include "BitsToInt_Block.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

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

BitsToInt_Block::BitsToInt_Block(const std::string& name)
    : Block(name)
    , m_numBits(4)
    , m_bitOrder(BitsToInt::MSB_first)
{
}

void BitsToInt_Block::SetDefaultParamters()
{
    m_numBits = 4;
    m_bitOrder = BitsToInt::MSB_first;
}

void BitsToInt_Block::SetParameters()
{
    if (!m_bitsToInt) {
        return;
    }

    m_bitsToInt->NumBits = m_numBits;
    m_bitsToInt->BitOrder = m_bitOrder;
}

bool BitsToInt_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool BitsToInt_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BitsToInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_bitsToInt = std::make_unique<BitsToInt>();

    SetDefaultParamters();
    try { m_numBits = std::stoi(getParameter("NumBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumBits', using default value."); }
    try { m_bitOrder = ConvertStringToBitOrder(getParameter("BitOrder").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BitOrder', using default value."); }

    if (m_numBits < 1) {
        m_numBits = 1;
    } else if (m_numBits > 32) {
        m_numBits = 32;
    }

    AddInputPort("input", m_bitsToInt->input, static_cast<size_t>(m_numBits), Block::DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_bitsToInt->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetParameters();

    return true;
}

BitsToInt::BitOrderEnum BitsToInt_Block::ConvertStringToBitOrder(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "lsb_first" || lower == "lsb first" || lower == "lsbfirst" || lower == "0") {
        return BitsToInt::LSB_first;
    }
    if (lower == "msb_first" || lower == "msb first" || lower == "msbfirst" || lower == "1") {
        return BitsToInt::MSB_first;
    }
    return BitsToInt::MSB_first;
}

bool BitsToInt_Block::DataStreamRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<bool>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    if (m_numBits < 1 || m_numBits > 32) {
        return false;
    }

    const size_t nbits = static_cast<size_t>(m_numBits);
    const size_t groups = inputData.size() / nbits;
    if (groups == 0) {
        return true;
    }

    std::vector<int> outputData;

    for (size_t g = 0; g < groups; ++g) {
        uint32_t acc = 0u;
        const size_t base = g * nbits;

        if (m_bitOrder == BitsToInt::LSB_first) {
            for (size_t i = 0; i < nbits; ++i) {
                const bool b = inputData[base + i];
                acc |= (static_cast<uint32_t>(b ? 1u : 0u) << i);
            }
        } else {
            for (size_t i = 0; i < nbits; ++i) {
                const bool b = inputData[base + i];
                acc = (acc << 1) | (b ? 1u : 0u);
            }
        }

        int out_val = 0;
        if (m_numBits == 32) {
            const int64_t val =
                (acc & 0x80000000u) ? (static_cast<int64_t>(acc) - 0x1'0000'0000LL)
                : static_cast<int64_t>(acc);
            out_val = static_cast<int>(val);
        } else {
            out_val = static_cast<int>(acc);
        }

        outputData.push_back(out_val);
    }
    WriteOutputData(outputPort, outputData);

    return true;
}

bool BitsToInt_Block::TimeDrivenRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<bool>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    for(size_t i = 0; i < inputData.size(); i++) {
        m_inputBuffer.push_back(inputData[i]);
    }
    if(m_inputBuffer.size() >= static_cast<size_t>(m_numBits)) {
        if (m_numBits < 1 || m_numBits > 32) {
            return false;
        }

        const size_t nbits = static_cast<size_t>(m_numBits);
        const size_t groups = inputData.size() / nbits;
        if (groups == 0) {
            return true;
        }

        std::vector<int> outputData;

        for (size_t g = 0; g < groups; ++g) {
            uint32_t acc = 0u;
            const size_t base = g * nbits;

            if (m_bitOrder == BitsToInt::LSB_first) {
                for (size_t i = 0; i < nbits; ++i) {
                    const bool b = inputData[base + i];
                    acc |= (static_cast<uint32_t>(b ? 1u : 0u) << i);
                }
            } else {
                for (size_t i = 0; i < nbits; ++i) {
                    const bool b = inputData[base + i];
                    acc = (acc << 1) | (b ? 1u : 0u);
                }
            }

            int out_val = 0;
            if (m_numBits == 32) {
                const int64_t val =
                    (acc & 0x80000000u) ? (static_cast<int64_t>(acc) - 0x1'0000'0000LL)
                    : static_cast<int64_t>(acc);
                out_val = static_cast<int>(val);
            } else {
                out_val = static_cast<int>(acc);
            }

            outputData.push_back(out_val);
        }

        for (const auto& val : outputData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
        if (!m_outputQueue.empty())
        {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}
