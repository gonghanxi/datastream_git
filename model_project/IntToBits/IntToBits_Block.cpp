#include "IntToBits_Block.h"

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

IntToBits_Block::IntToBits_Block(const std::string &name)
    :Block(name)
{

}

bool IntToBits_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool IntToBits_Block::Run()
{
    const std::string inputPort = GetInputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    if (m_numBits < 1 || m_numBits > 32) {
        return false;
    }

    const size_t nbits = static_cast<size_t>(m_numBits);
    std::vector<int> outputData;
    outputData.reserve(inputData.size() * nbits);

    for (const int& val : inputData) {
        const unsigned int uval = static_cast<unsigned int>(val);

        if (m_bitOrder == IntToBits::LSB_first) {
            // LSB first: 先输出LSB，最后输出MSB
            for (size_t k = 0; k < nbits; ++k) {
                const int b = ((uval >> k) & 0x1u) ? 1 : 0;
                outputData.push_back(b);
            }
        } else { // MSB_first
            // MSB first: 先输出MSB，最后输出LSB
            for (size_t k = 0; k < nbits; ++k) {
                const int b = ((uval >> (nbits - 1 - k)) & 0x1u) ? 1 : 0;
                outputData.push_back(b);
            }
        }
    }
    if(IsVariableStepMode()) return TimeDrivenRun(outputData);
    return DataStreamRun(outputData);

}

bool IntToBits_Block::DataStreamRun(std::vector<int> outputData)
{
    const std::string outputPort = GetOutputPortName(0);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool IntToBits_Block::TimeDrivenRun(std::vector<int> outputData)
{
    const std::string outputPort = GetOutputPortName(0);
    for(const auto& val : outputData) {
        m_outputQueue.push(val);
    }
    if (!m_outputQueue.empty())
    {
        int outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[IntToBits_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
    }
    return true;
}

bool IntToBits_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_IntTobits = std::make_unique<IntToBits>();

    SetDefaultParamters();
    try { m_numBits = std::stoi(getParameter("NumBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumBits', using default value."); }
    try { m_bitOrder = ConvertStringToBitOrder(getParameter("BitOrder").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BitOrder', using default value."); }

    if (m_numBits < 1) {
        m_numBits = 1;
    } else if (m_numBits > 32) {
        m_numBits = 32;
    }

    AddInputPort("input", m_IntTobits->input, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_IntTobits->output, static_cast<size_t>(m_numBits), Block::DataType::CIRCULAR_BUFFER_INT);

    SetParameters();

    return true;
}

void IntToBits_Block::SetDefaultParamters()
{
    m_numBits = 4;
    m_bitOrder = IntToBits::MSB_first;
}

void IntToBits_Block::SetParameters()
{
    if (!m_IntTobits) {
        return;
    }

    m_IntTobits->NumBits = m_numBits;
    m_IntTobits->BitOrder = m_bitOrder;
}

IntToBits::BitOrderEnum IntToBits_Block::ConvertStringToBitOrder(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "lsb_first" || lower == "lsb first" || lower == "lsbfirst" || lower == "0") {
        return IntToBits::LSB_first;
    }
    if (lower == "msb_first" || lower == "msb first" || lower == "msbfirst" || lower == "1") {
        return IntToBits::MSB_first;
    }
    return IntToBits::MSB_first;
}


