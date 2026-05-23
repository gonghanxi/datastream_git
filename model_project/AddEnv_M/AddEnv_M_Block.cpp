#include "AddEnv_M_Block.h"

AddEnv_M_Block::AddEnv_M_Block(const std::string &name)
    :Block(name)
{

}

bool AddEnv_M_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

bool AddEnv_M_Block::Run()
{
    //时间驱动
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool AddEnv_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_add = std::make_unique<AddEnv_M>();
    AddInputPort("input", m_add->input, 1, DataType::MATRIX_ENVELOPE_BUS);
    AddOutputPort("output", m_add->output, 1, DataType::MATRIX_ENVELOPE);
    return true;
}

bool AddEnv_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<EnvelopeMatrix>(GetInputPortName(0));
    std::vector<EnvelopeMatrix> outputData;

    ChannelNum = inputData.size();
    EnvelopeMatrix& firstMatrix = inputData[0];
    EnvelopeMatrix outMatrix;

    int NRow = firstMatrix.NumRows();
    int NCol = firstMatrix.NumColumns();

    outMatrix.Resize(NRow, NCol);
    outMatrix.Zero();
    for (int i = 0; i < ChannelNum; i++)
    {
        outMatrix += inputData[i];
    }
    outputData.push_back(outMatrix);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool AddEnv_M_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    size_t maxWriteSize = 0;
    //保证多输入同时读取数据
    for(const auto& bridge_reader : bridge_readers) {
        maxWriteSize = std::max(maxWriteSize, bridge_reader.bridgeReader->GetConnectedBuffer()->GetWriteSize());

        std::vector<EnvelopeMatrix> inputData;
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
        if(it->second.size() != maxWriteSize) {
            CanProcessData = false;
            break;
        }
    }

    std::vector<EnvelopeMatrix> outputData(maxWriteSize);

    if(CanProcessData && maxWriteSize > 0) {
        // 遍历每个位置
        for(size_t i = 0; i < maxWriteSize; ++i) {
            EnvelopeMatrix sum;

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
            EnvelopeMatrix outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<EnvelopeMatrix>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[AddEnv_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue(0,0).real() << "," << outputValue(0,0).imag();
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}
