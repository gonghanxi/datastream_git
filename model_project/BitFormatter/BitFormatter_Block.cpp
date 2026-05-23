#include "BitFormatter_Block.h"

BitFormatter_Block::BitFormatter_Block(const std::string& name)
    :Block(name)
{

}

bool BitFormatter_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool BitFormatter_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BitFormatter_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_bitformatter = std::make_unique<BitFormatter>();

    AddInputPort("input" , m_bitformatter->input,1 , DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_bitformatter->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParameters();

    try { m_SamplesPerBit = std::stoi(getParameter("SamplesPerBit").Value); } catch (...) {}
    try { m_Format = ConvertStringToSelectedFormat(getParameter("Format").Value); } catch (...) {}
    try { m_LogicZeroLevel = std::stod(getParameter("LogicZeroLevel").Value); } catch (...) {}
    try { m_LogicOneLevel = std::stod(getParameter("LogicOneLevel").Value); } catch (...) {}


    SetParameters(m_SamplesPerBit, m_Format, m_LogicZeroLevel, m_LogicOneLevel);

    m_bitformatter->Setup();

    GetOutputPort(GetOutputPortName(0))->SetWriteSize(m_SamplesPerBit);

    return true;
}

void BitFormatter_Block::SetParameters(int samplesperbit, BitFormatter::SelectedFormat format, double logiczerolevel, double logiconelevel)
{
    if(m_bitformatter) {
        m_bitformatter->SamplesPerBit = samplesperbit;
        m_bitformatter->Format = format;
        m_bitformatter->LogicZeroLevel = logiczerolevel;
        m_bitformatter->LogicOneLevel = logiconelevel;
    }
}

BitFormatter::SelectedFormat BitFormatter_Block::ConvertStringToSelectedFormat(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "nrz" || lowerValue == "0") {
        return BitFormatter::NRZ;
    } else if (lowerValue == "rz" || lowerValue == "1") {
        return BitFormatter::RZ;
    }
    return BitFormatter::NRZ;
}

void BitFormatter_Block::SetDefaultParameters()
{
    m_SamplesPerBit = 1;
    m_Format = BitFormatter::NRZ;
    m_LogicZeroLevel = -1;
    m_LogicOneLevel = 1;
}

bool BitFormatter_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::vector<bool> inputData = ReadInputData<bool>(inputPortName);

    if (inputData.empty()) {
        std::cout << "ERROR: No input data available" << std::endl;
        return false;
    }
    std::string outputPortName = GetOutputPortName(0);
    std::vector<double> outputData(m_SamplesPerBit);
    outputData.resize(m_SamplesPerBit);
    for(size_t i = 0; i < inputData.size(); i++) {
        double outBit = inputData[i] ? m_LogicOneLevel : m_LogicZeroLevel;
        for (int j = 0; j < m_SamplesPerBit; j++)
        {
            outputData[j] = outBit;

            if (m_Format == BitFormatter::RZ && j >= m_SamplesPerBit / 2)
            {
                outputData[j] = 0;
            }
        }
    }

    WriteOutputData(outputPortName, outputData);
    return true;
}

bool BitFormatter_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::vector<bool> inputData = ReadInputData<bool>(inputPortName);

    if (inputData.empty()) return true;
    m_inputBuffer.push_back(inputData[0]);

    std::vector<double> outputData(m_SamplesPerBit);
    outputData.resize(m_SamplesPerBit);

    double outBit = m_inputBuffer[0] ? m_LogicOneLevel : m_LogicZeroLevel;
    for (int j = 0; j < m_SamplesPerBit; j++)
    {
        outputData[j] = outBit;

        if (m_Format == BitFormatter::RZ && j >= m_SamplesPerBit / 2)
        {
            outputData[j] = 0;
        }
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
    return true;
}
