#include "BitDeformatter_Block.h"
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
BitDeformatter_Block::BitDeformatter_Block(const std::string &name)
    :Block(name)
{

}

bool BitDeformatter_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool BitDeformatter_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BitDeformatter_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_BitDeformatter = std::make_unique<BitDeformatter>();
    SetDefaultParameters();
    try { m_Format = ConvertStringToSelectedFormat(getParameter("Format").Value); } catch (...) {}
    try { m_SamplesPerBit = std::stoi(getParameter("SamplesPerBit").Value); } catch (...) {}
    try { m_LogicZeroLevel = std::stod(getParameter("LogicZeroLevel").Value); } catch (...) {}
    try { m_LogicOneLevel = std::stod(getParameter("LogicOneLevel").Value); } catch (...) {}
    SetParameters();
    if(!m_BitDeformatter->Setup()) return false;
    AddInputPort("input", m_BitDeformatter->input, static_cast<size_t>(m_SamplesPerBit), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_BitDeformatter->output, 1, DataType::CIRCULAR_BUFFER_BOOL);
    return true;
}

void BitDeformatter_Block::SetParameters()
{
    if(!m_BitDeformatter) return;
    m_BitDeformatter->Format = m_Format;
    m_BitDeformatter->SamplesPerBit = m_SamplesPerBit;
    m_BitDeformatter->LogicZeroLevel = m_LogicZeroLevel;
    m_BitDeformatter->LogicOneLevel = m_LogicOneLevel;
}

BitDeformatter::SelectedFormat BitDeformatter_Block::ConvertStringToSelectedFormat(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "nrz" || lower == "0") {
        return BitDeformatter::NRZ;
    }
    if (lower == "rz" || lower == "1") {
        return BitDeformatter::RZ;
    }
    return BitDeformatter::NRZ;
}

void BitDeformatter_Block::SetDefaultParameters()
{
    m_Format = BitDeformatter::NRZ;
    m_SamplesPerBit = 1;
    m_LogicZeroLevel = -1;
    m_LogicOneLevel = 1;
}

bool BitDeformatter_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<bool> outputData(1);
    outputData.reserve(1);
    if (m_Format == BitDeformatter::NRZ)
    {
        double bitAverage = 0;
        for (int i = 0; i < m_SamplesPerBit; i++)
        {
            bitAverage += inputData[i];
        }
        bitAverage /= m_SamplesPerBit;

        outputData[0] = (std::abs(bitAverage - m_LogicZeroLevel) < std::abs(bitAverage - m_LogicOneLevel)) ? 0 : 1;
    }

    else if (m_Format == BitDeformatter::RZ)
    {
        double bitZero = 0;
        double bitAverage = 0;
        for (int i = 0; i < m_SamplesPerBit; i++)
        {
            if (i < m_SamplesPerBit / 2)
            {
                bitAverage += inputData[i];
            }
            else
            {
                bitZero += inputData[i];
            }
        }
        bitZero /= m_SamplesPerBit / 2;
        bitAverage /= m_SamplesPerBit / 2;
        bitAverage -= bitZero;

        outputData[0] = (std::abs(bitAverage - m_LogicZeroLevel) < std::abs(bitAverage - m_LogicOneLevel)) ? 0 : 1;
    }
    WriteOutputData(outputPort, outputData);
    return true;
}

bool BitDeformatter_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);

    if(inputData.empty()) return true;
    for(size_t i = 0; i < inputData.size(); i++) m_inputBuffer.push_back(inputData[i]);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_SamplesPerBit)) {
        std::vector<bool> outputData(1);
        if (m_Format == BitDeformatter::NRZ)
        {
            double bitAverage = 0;
            for (int i = 0; i < m_SamplesPerBit; i++)
            {
                bitAverage += m_inputBuffer[i];
            }
            bitAverage /= m_SamplesPerBit;

            outputData[0] = (std::abs(bitAverage - m_LogicZeroLevel) < std::abs(bitAverage - m_LogicOneLevel)) ? 0 : 1;
        }

        else if (m_Format == BitDeformatter::RZ)
        {
            double bitZero = 0;
            double bitAverage = 0;
            for (int i = 0; i < m_SamplesPerBit; i++)
            {
                if (i < m_SamplesPerBit / 2)
                {
                    bitAverage += m_inputBuffer[i];
                }
                else
                {
                    bitZero += m_inputBuffer[i];
                }
            }
            bitZero /= m_SamplesPerBit / 2;
            bitAverage /= m_SamplesPerBit / 2;
            bitAverage -= bitZero;

            outputData[0] = (std::abs(bitAverage - m_LogicZeroLevel) < std::abs(bitAverage - m_LogicOneLevel)) ? 0 : 1;
        }
        m_outputQueue.push(outputData[0]);
        m_inputBuffer.clear();
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
