#include "Sub_Block.h"

Sub_Block::Sub_Block(const std::string& name)
	: Block(name)
{
}

void Sub_Block::SetDefaultParamters()
{
}

bool Sub_Block::DataStreamRun()
{
    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto posData = ReadInputData<double>(posPort);
    if (posData.empty()) {
        return false;
    }

    auto negData = ReadInputData<double>(negPort);

    double acc = posData[0];
    for (size_t i = 0; i < negData.size(); ++i) {
        acc -= negData[i];
    }

    std::vector<double> outputData;
    outputData.push_back(acc);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool Sub_Block::TimeDrivenRun()
{
    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(negPort);
    auto bridge_readers = master_reader->GetBusConnections();

    for(const auto& bridge_reader : bridge_readers) {
        std::vector<double> negData;
        bridge_reader.bridgeReader->ReadData(negData);
        if(negData.empty()) {
            return true;
        }
        for(size_t i = 0; i < negData.size();i++) {
            m_negBuffer[bridge_reader.bridgeReader].push_back(negData[i]);
        }
    }

    auto posData = ReadInputData<double>(posPort);
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
        std::vector<double> outputData;
        outputData.push_back(acc);
        m_outputQueue.push(outputData[0]);

        if (!m_outputQueue.empty()) {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<double>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[Sub_Block] :" << m_outputCount
                     << " value:" << outputValue;
            m_posBuffer.clear();
            m_negBuffer.clear();
        }
    }
    return true;
}

bool Sub_Block::Setup()
{
	Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool Sub_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Sub_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

    m_sub = std::make_unique<Sub>();

	AddInputPort("pos", m_sub->pos, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddInputPort("neg", m_sub->neg, 1, Block::DataType::DOUBLE_BUS);
	AddOutputPort("output", m_sub->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

	return true;
}
