#include "Gain_Block.h"

//template class GAIN_API std::map<std::string, Parameter>;
//template class GAIN_API std::map<int, PortMsg>;

Gain_Block::Gain_Block(const std::string &name)
    :Block(name), m_gain(1.0)
{
}

void Gain_Block::SetParameters(double gain)
{
    m_gain = gain;
    if(m_Gain) {
        m_Gain->m_Gain = gain;
    }
}

bool Gain_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Gain_Block::Run()
{
    // 获取输入输出端口名称
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    //----------------读取数据---------------------
    std::vector<double> inputData = ReadInputData<double>(inputPortName);
    //时间驱动 与 数据流驱动 区别
    //时间驱动读取可用数据，若读取为空，跳过此步长执行
    //数据流驱动读取读指针数据，若读取为空，则表示数据没有到达，返回错误
    if(inputData.empty()) {
        if(IsVariableStepMode()) {
            return true;
        }
        return false;
    }
    qDebug() << "Gain_Block::Run - inputData: " << inputData.size();

    //----------------数据处理---------------------
    std::vector<double> outputData;
    outputData.reserve(inputData.size());
    for(size_t i = 0; i < inputData.size(); i++) {
        double outputSample = m_gain * inputData[i];
        outputData.push_back(outputSample);
    }
    //----------------数据处理---------------------
    //----------------写入数据---------------------
    WriteOutputData(outputPortName, outputData);
    return true;
}
bool Gain_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Gain = std::make_unique<Gain>();

    AddInputPort("input", m_Gain->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Gain->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    //在引擎之前默认参数
    SetDefaultParameters();
    //从引擎获取参数
    m_gain = std::stod(getParameter("m_Gain").Value);
    //获取到引擎后设置给模型的参数
    SetParameters(m_gain);
    return true;
}

void Gain_Block::SetDefaultParameters()
{
    m_gain = 1.0;
}




