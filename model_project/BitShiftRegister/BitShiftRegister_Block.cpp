#include "BitShiftRegister_Block.h"

#include <algorithm>
#include <cctype>

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

BitShiftRegister_Block::BitShiftRegister_Block(const std::string& name)
    : Block(name)
    , m_numBits(8)
    , m_bitOrder(BitShiftRegister::MSB_FIRST)
{
}

void BitShiftRegister_Block::SetDefaultParamters()
{
    m_numBits = 8;
    m_bitOrder = BitShiftRegister::MSB_FIRST;
}

void BitShiftRegister_Block::SetParameters()
{
    if (!m_bitShiftRegister) {
        return;
    }

    m_bitShiftRegister->NumBits = m_numBits;
    m_bitShiftRegister->BitOrder = m_bitOrder;
}

bool BitShiftRegister_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    SetIsBitShiftRegister(true);
    SetBitShiftRegisterNumBits(m_numBits);
    return true;
}

bool BitShiftRegister_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BitShiftRegister_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_bitShiftRegister = std::make_unique<BitShiftRegister>();



    SetDefaultParamters();
    try { m_numBits = std::stoi(getParameter("NumBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumBits', using default value."); }
    try { m_bitOrder = ConvertStringToBitOrder(getParameter("BitOrder").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BitOrder', using default value."); }

    if (m_numBits <= 0) {
        m_numBits = 1;
    }

    AddInputPort("input", m_bitShiftRegister->input, 1, Block::DataType::TIMED_BOOL);
    AddInputPort("clock", m_bitShiftRegister->clock, 1, Block::DataType::TIMED_INT);
    AddInputPort("reset", m_bitShiftRegister->reset, 1, Block::DataType::TIMED_INT);
    AddOutputPort("output", m_bitShiftRegister->output, static_cast<int>(m_numBits), Block::DataType::TIMED_BOOL);

    SetParameters();

    m_reg.assign(static_cast<size_t>(m_numBits), 0);

    return true;
}

BitShiftRegister::BitOrderEnum BitShiftRegister_Block::ConvertStringToBitOrder(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "lsb_first" || lower == "lsb first" || lower == "lsbfirst" || lower == "0") {
        return BitShiftRegister::LSB_FIRST;
    }
    if (lower == "msb_first" || lower == "msb first" || lower == "msbfirst" || lower == "1") {
        return BitShiftRegister::MSB_FIRST;
    }
    return BitShiftRegister::MSB_FIRST;
}

bool BitShiftRegister_Block::DataStreamRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string clockPort = GetInputPortName(1);
    const std::string resetPort = GetInputPortName(2);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<bool>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    const bool hasClock = GetInputPort(GetInputPortName(1))->IsConnected();
    const bool hasReset = GetInputPort(GetInputPortName(2))->IsConnected();

    std::vector<int> clockData;
    std::vector<int> resetData;

    if (hasClock) {
        clockData = ReadInputData<int>(clockPort);
    }
    if (hasReset) {
        resetData = ReadInputData<int>(resetPort);
    }

    std::vector<bool> outputData;
    outputData.reserve(inputData.size() * static_cast<size_t>(m_numBits));

    for (size_t i = 0; i < inputData.size(); ++i) {
        const int inBit = inputData[i] ? 1 : 0;

        int clk = 1;
        int rst = 0;

        if (hasClock && !clockData.empty()) {
            const size_t ci = (i < clockData.size()) ? i : (clockData.size() - 1);
            clk = clockData[ci];
        }

        if (hasReset && !resetData.empty()) {
            const size_t ri = (i < resetData.size()) ? i : (resetData.size() - 1);
            rst = resetData[ri];
        }

        if (rst != 0) {
            std::fill(m_reg.begin(), m_reg.end(), 0);
        } else if (!hasClock || clk != 0) {
            const int N = static_cast<int>(m_reg.size());
            if (N > 0) {
                for (int j = N - 1; j > 0; --j) {
                    m_reg[j] = m_reg[j - 1];
                }
                m_reg[0] = inBit;
            }
        }

        const int Nbits = static_cast<int>(m_reg.size());
        if (Nbits <= 0) {
            continue;
        }

        if (m_bitOrder == BitShiftRegister::LSB_FIRST) {
            for (int j = 0; j < Nbits; ++j) {
                outputData.push_back(m_reg[j] != 0);
            }
        } else {
            for (int j = 0; j < Nbits; ++j) {
                outputData.push_back(m_reg[Nbits - 1 - j] != 0);
            }
        }
    }
    WriteOutputData(outputPort, outputData);
    return true;
}

bool BitShiftRegister_Block::TimeDrivenRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string clockPort = GetInputPortName(1);
    const std::string resetPort = GetInputPortName(2);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<bool>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    m_inputBuffer.push_back(inputData[0]);

    const bool hasClock = GetInputPort(GetInputPortName(1))->IsConnected();
    const bool hasReset = GetInputPort(GetInputPortName(2))->IsConnected();

    std::vector<int> clockData;
    std::vector<int> resetData;

    if (hasClock) {
        clockData = ReadInputData<int>(clockPort);
        m_clockBuffer.push_back(clockData[0]);
    }
    if (hasReset) {
        resetData = ReadInputData<int>(resetPort);
        m_resetBuffer.push_back(resetData[0]);
    }

    bool CanProcessData = false;

    if(inputData.size() >= 1) {
        bool clockReady = !hasClock || (clockData.size() >= 1);
        bool resetReady = !hasReset || (resetData.size() >= 1);

        if(clockReady && resetReady) {
            CanProcessData = true;
        }
    }

    if(CanProcessData) {
        std::vector<bool> outputData;
        outputData.reserve(static_cast<size_t>(m_numBits));

        for (size_t i = 0; i < m_inputBuffer.size(); ++i) {
            const int inBit = m_inputBuffer[i] ? 1 : 0;

            int clk = 1;
            int rst = 0;

            if (hasClock && !m_clockBuffer.empty()) {
                const size_t ci = (i < m_clockBuffer.size()) ? i : (m_clockBuffer.size() - 1);
                clk = m_clockBuffer[ci];
            }

            if (hasReset && !m_resetBuffer.empty()) {
                const size_t ri = (i < m_resetBuffer.size()) ? i : (m_resetBuffer.size() - 1);
                rst = m_resetBuffer[ri];
            }

            if (rst != 0) {
                std::fill(m_reg.begin(), m_reg.end(), 0);
            } else if (!hasClock || clk != 0) {
                const int N = static_cast<int>(m_reg.size());
                if (N > 0) {
                    for (int j = N - 1; j > 0; --j) {
                        m_reg[j] = m_reg[j - 1];
                    }
                    m_reg[0] = inBit;
                }
            }

            const int Nbits = static_cast<int>(m_reg.size());
            if (Nbits <= 0) {
                continue;
            }

            if (m_bitOrder == BitShiftRegister::LSB_FIRST) {
                for (int j = 0; j < Nbits; ++j) {
                    outputData.push_back(m_reg[j] != 0);
                }
            } else {
                for (int j = 0; j < Nbits; ++j) {
                    outputData.push_back(m_reg[Nbits - 1 - j] != 0);
                }
            }
        }
        for (const auto& val : outputData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
        m_clockBuffer.clear();
        m_resetBuffer.clear();
        if (!m_outputQueue.empty())
        {
            bool outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<bool>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}


