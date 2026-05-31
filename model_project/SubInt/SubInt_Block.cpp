#include "SubInt_Block.h"

SubInt_Block::SubInt_Block(const std::string& name)
    : Block(name)
{
}

void SubInt_Block::SetDefaultParamters()
{
}

bool SubInt_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool SubInt_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool SubInt_Block::DataStreamRun()
{
    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto posData = ReadInputData<int>(posPort);
    if (posData.empty()) {
        return false;
    }

    auto negData = ReadInputData<int>(negPort);

    double acc = posData[0];
    for (size_t i = 0; i < negData.size(); ++i) {
        acc -= negData[i];
    }

    std::vector<int> outputData;
    outputData.push_back(acc);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool SubInt_Block::TimeDrivenRun()
{
    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(negPort);
    auto bridge_readers = master_reader->GetBusConnections();

    for(const auto& bridge_reader : bridge_readers) {
        std::vector<int> negData;
        bridge_reader.bridgeReader->ReadData(negData);
        if(negData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < negData.size();i++) {
            m_negBuffer[bridge_reader.bridgeReader].push_back(negData[i]);
        }
    }

    auto posData = ReadInputData<int>(posPort);
    if (posData.empty()) {
        return true;
    }
    m_posBuffer.push_back(posData[0]);

    bool CanProcessData = true;
    for(auto it = m_negBuffer.begin(); it != m_negBuffer.end(); ++it) {
        if(it->second.size() >= 1) {
            CanProcessData = false;
            break;
        }
    }
    if(m_posBuffer.size() < 1) {
        CanProcessData = false;
    }
    if(CanProcessData) {
        double acc = m_posBuffer[0];
        for(auto it = m_negBuffer.begin(); it != m_negBuffer.end(); ++it) {
            acc -= it->second[0];
        }
        std::vector<int> outputData;
        outputData.push_back(acc);
        m_outputQueue.push(outputData[0]);

        if (!m_outputQueue.empty()) {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<int>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[SubInt_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
            m_posBuffer.clear();
            m_negBuffer.clear();
        }
    }
    return true;
}

bool SubInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_subInt = std::make_unique<SubInt>();

    AddInputPort("pos", m_subInt->pos, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddInputPort("neg", m_subInt->neg, 1, Block::DataType::INT_BUS);
    AddOutputPort("output", m_subInt->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetDefaultParamters();

    return true;
}
