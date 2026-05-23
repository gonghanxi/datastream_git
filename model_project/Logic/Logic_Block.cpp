#include "Logic_Block.h"
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
Logic_Block::Logic_Block(const std::string& name)
    :Block(name)
{

}

bool Logic_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Logic_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<bool>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    std::vector<bool> outputData(1);
    outputData.reserve(inputData.size());

    bool bStatus = true;
    int	ChannelNumIn = inputData.size();
    qDebug() << "ChannelNumIn: " << ChannelNumIn;

    outputData[0] = inputData[0]; // ȡ��һ������ֵ��ʼ��

    switch (m_LogicOperation)
    {
    case Logic::NOT:
        if (ChannelNumIn != 1)
        {
            LOG_ERROR("NOT operatuon can only have one input.");
            bStatus = false;
        }
        outputData[0] = !(inputData[0]);
        break;
    case Logic::AND:
        for (int i = 1; i < ChannelNumIn; i++)
        {
            outputData[0] = outputData[0] && inputData[i];
        }
        break;
    case Logic::NAND:
        for (int i = 1; i < ChannelNumIn; i++)
        {
            outputData[0] = outputData[0] && inputData[i];
        }
        outputData[0] = !(outputData[0]);
        break;
    case Logic::OR:
        for (int i = 1; i < ChannelNumIn; i++)
        {
            outputData[0] = outputData[0] || inputData[i];
        }
        break;
    case Logic::NOR:
        for (int i = 1; i < ChannelNumIn; i++)
        {
            outputData[0] = outputData[0] || inputData[i];
        }
        outputData[0] = !(outputData[0]);
        break;
    case Logic::XOR:
        for (int i = 1; i < ChannelNumIn; i++)
        {
            outputData[0] = outputData[0] ^ inputData[i];
        }
        break;
    case Logic::XNOR:
        for (int i = 1; i < ChannelNumIn; i++)
        {
            outputData[0] = outputData[0] ^ inputData[i];
        }
        outputData[0] = !(outputData[0]);
        break;
    default:
        break;
    }
    for(size_t i = 0; i < outputData.size(); i++) {
        qDebug() << "outputData: " << outputData[i];
    }
    WriteOutputData(outputPort, outputData);
    return bStatus;
}

bool Logic_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Logic = std::make_unique<Logic>();


    SetDefaultParameters();

    try { m_LogicOperation = ConvertStringToSelectedLogicOperation(getParameter("LogicOperation").Value); } catch (...) { }

    SetParameters();

    AddInputPort("input", m_Logic->input, 1, DataType::BOOL_BUS);
    AddOutputPort("output", m_Logic->output, 1, DataType::CIRCULAR_BUFFER_BOOL);

    return true;
}

void Logic_Block::SetParameters()
{
    if(!m_Logic) return;
    m_Logic->LogicOperation = m_LogicOperation;
}

Logic::SelectedLogicOperation Logic_Block::ConvertStringToSelectedLogicOperation(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "not" || lower == "0") {
        return Logic::NOT;
    }
    if (lower == "and" || lower == "1") {
        return Logic::AND;
    }
    if (lower == "nand" || lower == "2") {
        return Logic::NAND;
    }
    if (lower == "or" || lower == "3") {
        return Logic::OR;
    }
    if (lower == "nor" || lower == "4") {
        return Logic::NOR;
    }
    if (lower == "xor" || lower == "5") {
        return Logic::XOR;
    }
    if (lower == "xnor" || lower == "6") {
        return Logic::XNOR;
    }
    return Logic::AND;
}

void Logic_Block::SetDefaultParameters()
{
    m_LogicOperation = Logic::AND;
}
