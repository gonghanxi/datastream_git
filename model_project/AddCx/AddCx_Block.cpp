#include "AddCx_Block.h"


AddCx_Block::AddCx_Block(const std::string& name)
    :Block(name)
{

}

bool AddCx_Block::Setup()
{
    Block::Setup();
    // 清空输出队列
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

// AddCx_Block.cpp
bool AddCx_Block::Run()
{
    //时间驱动
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool AddCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_addCx = std::make_unique<AddCx>();

    AddInputPort("input", m_addCx->input, 1, DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_addCx->output, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    GetOutputPort(GetOutputPortName(0))->SetWriteSize(1U);

    return true;
}

bool AddCx_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPortName);



    // 累加所有输入数据
    std::complex<double> acc(0.0, 0.0);
    for (size_t i = 0; i < inputData.size(); ++i) {
        acc += inputData[i];
    }
    // 写入输出
    std::string outputPortName = GetOutputPortName(0);
    std::vector<std::complex<double>> outputData;
    outputData.push_back(acc);

    WriteOutputData(outputPortName, outputData);


    return true;
}

bool AddCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    //保证多输入同时读取数据
    for(const auto& bridge_reader : bridge_readers) {

        std::vector<std::complex<double>> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if(inputData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < inputData.size();i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    bool CanProcessData = true;
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() >= 1) {
            CanProcessData = false;
            break;
        }
    }

    std::vector<std::complex<double>> outputData(1, 0.0);  // 初始化为0

    if(CanProcessData) {
        // 遍历每个位置
        for(size_t i = 0; i < 1; ++i) {
            std::complex<double> sum(0.0,0.0);

            // 遍历每个缓冲区，累加第i个元素
            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                sum += it->second[i];
            }

            outputData[i] = sum;
            //将处理结果放入输出队列
            m_outputQueue.push(outputData[i]);
        }
        //执行写入
        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[AddCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}
