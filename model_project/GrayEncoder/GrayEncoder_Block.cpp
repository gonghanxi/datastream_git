#include "GrayEncoder_Block.h"
#include <cstring>
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
GrayEncoder_Block::GrayEncoder_Block(const std::string &name)
    :Block(name)
    , m_inBits(nullptr)
    , m_outBits(nullptr)
    , m_lastOutput(false)
    , m_inputCount(0)
    , m_outputCount(0)
{

}

GrayEncoder_Block::~GrayEncoder_Block()
{
    FreeBuffersBlock();
}

void GrayEncoder_Block::FreeBuffersBlock()
{
    delete[] m_inBits;  m_inBits = nullptr;
    delete[] m_outBits; m_outBits = nullptr;
}

bool GrayEncoder_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();

    if (NumBits <= 0)
    {
        LOG_ERROR("GrayEncoder: NumBits must be > 0");
        NumBits = 1;
    }

    FreeBuffersBlock();
    m_inBits = new bool[NumBits];
    m_outBits = new bool[NumBits];
    std::memset(m_inBits, 0, sizeof(bool) * NumBits);
    std::memset(m_outBits, 0, sizeof(bool) * NumBits);

    return true;
}

bool GrayEncoder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool GrayEncoder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_gray = std::make_unique<GrayEncoder>();
    SetDefaultParameters();
    try { NumBits = std::stoi(getParameter("NumBits").Value); } catch (...) {}
    try { m_BitOrder = ConvertStringToBitOrderE(getParameter("BitOrder").Value); } catch (...) {}
    SetParameters();

    if (NumBits <= 0)
    {
        LOG_ERROR("GrayEncoder: NumBits must be > 0");
        NumBits = 1;
    }

    AddInputPort("input", m_gray->input, static_cast<size_t>(NumBits), DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_gray->output, static_cast<size_t>(NumBits), DataType::CIRCULAR_BUFFER_BOOL);
    return true;
}

void GrayEncoder_Block::SetParameters()
{
    if(!m_gray) return;
    m_gray->m_BitOrder = m_BitOrder;
    m_gray->NumBits = NumBits;
}

void GrayEncoder_Block::SetDefaultParameters()
{
    m_BitOrder = GrayEncoder::MSB_first;
    NumBits = 4;
}

GrayEncoder::BitOrder GrayEncoder_Block::ConvertStringToBitOrderE(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "lsb_first" || lower == "0") return GrayEncoder::LSB_first;
    if(lower == "msb_first" || lower == "1") return GrayEncoder::MSB_first;
    return GrayEncoder::MSB_first;
}

bool GrayEncoder_Block::DataStreamRun()
{
    std::vector<bool> inputData = ReadInputData<bool>(GetInputPortName(0));
    std::vector<bool> outputData(static_cast<size_t>(NumBits));

    if (m_BitOrder == GrayEncoder::MSB_first)
    {
        for (int k = 0; k < NumBits; ++k)
            m_inBits[NumBits - 1 - k] = inputData[k];
    }
    else
    {
        for (int k = 0; k < NumBits; ++k)
            m_inBits[k] = inputData[k];
    }

    GrayEncodeBitsLSB0Block(m_inBits, m_outBits, NumBits);

    if (m_BitOrder == GrayEncoder::MSB_first)
    {
        for (int k = 0; k < NumBits; ++k)
            outputData[k] = m_outBits[NumBits - 1 - k];
    }
    else
    {
        for (int k = 0; k < NumBits; ++k)
            outputData[k] = m_outBits[k];
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool GrayEncoder_Block::TimeDrivenRun()
{
    std::vector<bool> inputData = ReadInputData<bool>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(NumBits)) {
        std::vector<bool> outputData(static_cast<size_t>(NumBits));

        if (m_BitOrder == GrayEncoder::MSB_first)
        {
            for (int k = 0; k < NumBits; ++k)
                m_inBits[NumBits - 1 - k] = m_inputBuffer[k];
        }
        else
        {
            for (int k = 0; k < NumBits; ++k)
                m_inBits[k] = m_inputBuffer[k];
        }

        GrayEncodeBitsLSB0Block(m_inBits, m_outBits, NumBits);

        if (m_BitOrder == GrayEncoder::MSB_first)
        {
            for (int k = 0; k < NumBits; ++k)
                outputData[k] = m_outBits[NumBits - 1 - k];
        }
        else
        {
            for (int k = 0; k < NumBits; ++k)
                outputData[k] = m_outBits[k];
        }
        for (const auto& val : outputData)
        {
            m_outputQueue.push(val);
        }
    }
    if (!m_outputQueue.empty())
    {
        bool outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(GetOutputPortName(0), std::vector<bool>{outputValue});
        m_lastOutput = outputValue;
        m_inputBuffer.clear();

        qDebug() << "[GrayEncoder_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
    }
    return true;
}

void GrayEncoder_Block::GrayEncodeBitsLSB0Block(const bool* binLSB0, bool* grayLSB0, int nbits)
{
    if (nbits <= 0) return;

    if (nbits == 1)
    {
        grayLSB0[0] = binLSB0[0];
        return;
    }

    const int msb = nbits - 1;
    grayLSB0[msb] = binLSB0[msb];
    for (int i = 0; i < msb; ++i)
    {
        grayLSB0[i] = (binLSB0[i] != binLSB0[i + 1]);
    }
}
