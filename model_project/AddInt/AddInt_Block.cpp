#include "AddInt_Block.h"

AddInt_Block::AddInt_Block(const std::string& name)
    : Block(name)
{
}

void AddInt_Block::SetDefaultParamters()
{
}

bool AddInt_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    long long acc = 0;
    for (size_t i = 0; i < inputData.size(); ++i) {
        acc += static_cast<long long>(inputData[i]);
    }

    std::vector<int> outputData;
    outputData.push_back(static_cast<int>(acc));

    WriteOutputData(outputPort, outputData);

    return true;
}

bool AddInt_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    for(const auto& bridge_reader : bridge_readers) {

        std::vector<int> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if(inputData.empty()) {
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

    std::vector<int> outputData(1);

    if(CanProcessData) {
        for(size_t i = 0; i < 1; ++i) {
            int sum = 0;

            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                sum += it->second[i];
            }

            outputData[i] = sum;
            m_outputQueue.push(outputData[i]);
        }
        //执行写入
        if (!m_outputQueue.empty()) {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<int>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[AddInt_Block] :" << m_outputCount
                     << " value:" << outputValue;
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

bool AddInt_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

bool AddInt_Block::Run()
{
    //时间驱动
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool AddInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_addInt = std::make_unique<AddInt>();

    m_addInt->output.SetRate(1U);

    AddInputPort("input", m_addInt->input, 1, Block::DataType::INT_BUS);
    AddOutputPort("output", m_addInt->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetDefaultParamters();

    return true;
}
