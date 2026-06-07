#include "GrayDecoder_Block.h"
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
GrayDecoder_Block::GrayDecoder_Block(const std::string &name)
    :Block(name)
    , inBits(nullptr)
    , outBits(nullptr)
    , m_lastOutput(false)
    , m_inputCount(0)
    , m_outputCount(0)
{

}

GrayDecoder_Block::~GrayDecoder_Block()
{
    FreeBuffersBlock();
}

void GrayDecoder_Block::FreeBuffersBlock()
{
    delete[] inBits;  inBits = nullptr;
    delete[] outBits; outBits = nullptr;
}
bool GrayDecoder_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    if (NumBits <= 0)
    {
        LOG_ERROR("GrayDecoder: NumBits must be > 0");
        NumBits = 1;
    }
    FreeBuffersBlock();
    inBits = new bool[NumBits];
    outBits = new bool[NumBits];
    std::memset(inBits, 0, sizeof(bool) * NumBits);
    std::memset(outBits, 0, sizeof(bool) * NumBits);
    return true;
}

bool GrayDecoder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool GrayDecoder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_gray = std::make_unique<GrayDecoder>();
    SetDefaultParameters();
    try { NumBits = std::stoi(getParameter("NumBits").Value); } catch (...) {}
    try { m_BitOrder = ConvertStringToBitOrderE(getParameter("BitOrder").Value); } catch (...) {}
    SetParameters();

    int n = (NumBits > 0) ? NumBits : 1;

    AddInputPort("input", m_gray->input, static_cast<size_t>(n), DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_gray->output, static_cast<size_t>(n), DataType::CIRCULAR_BUFFER_BOOL);
    return true;
}

void GrayDecoder_Block::SetParameters()
{
    if(!m_gray) return;
    m_gray->m_BitOrder = m_BitOrder;
    m_gray->NumBits = NumBits;
}

void GrayDecoder_Block::SetDefaultParameters()
{
    m_BitOrder = GrayDecoder::MSB_first;
    NumBits = 4;
}

GrayDecoder::BitOrderE GrayDecoder_Block::ConvertStringToBitOrderE(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "lsb_first" || lower == "0") return GrayDecoder::LSB_first;
    if(lower == "msb_first" || lower == "1") return GrayDecoder::MSB_first;
    return GrayDecoder::MSB_first;
}

bool GrayDecoder_Block::DataStreamRun()
{
    std::vector<bool> inputData = ReadInputData<bool>(GetInputPortName(0));
    const int n = (NumBits > 0) ? NumBits : 1;
    std::vector<bool> outputData(static_cast<size_t>(n));

    if (m_BitOrder == GrayDecoder::MSB_first)
    {
        for (int k = 0; k < n; ++k)
            inBits[n - 1 - k] = inputData[k];
    }
    else
    {
        for (int k = 0; k < n; ++k)
            inBits[k] = inputData[k];
    }

    outBits[n - 1] = inBits[n - 1];
    for (int i = n - 2; i >= 0; --i)
        outBits[i] = (outBits[i + 1] ^ inBits[i]);

    if (m_BitOrder == GrayDecoder::MSB_first)
    {
        for (int k = 0; k < n; ++k)
            outputData[k] = outBits[n - 1 - k];
    }
    else
    {
        for (int k = 0; k < n; ++k)
            outputData[k] = outBits[k];
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool GrayDecoder_Block::TimeDrivenRun()
{
    std::vector<bool> inputData = ReadInputData<bool>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    size_t inputRate = GetInputPort(GetInputPortName(0))->GetReadSize();

    if(m_inputBuffer.size() >= inputRate) {
        const int n = (NumBits > 0) ? NumBits : 1;
        std::vector<bool> outputData(static_cast<size_t>(n));

        if (m_BitOrder == GrayDecoder::MSB_first)
        {
            for (int k = 0; k < n; ++k)
                inBits[n - 1 - k] = m_inputBuffer[k];
        }
        else
        {
            for (int k = 0; k < n; ++k)
                inBits[k] = m_inputBuffer[k];
        }

        outBits[n - 1] = inBits[n - 1];
        for (int i = n - 2; i >= 0; --i)
            outBits[i] = (outBits[i + 1] ^ inBits[i]);

        if (m_BitOrder == GrayDecoder::MSB_first)
        {
            for (int k = 0; k < n; ++k)
                outputData[k] = outBits[n - 1 - k];
        }
        else
        {
            for (int k = 0; k < n; ++k)
                outputData[k] = outBits[k];
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
    }
    return true;
}
