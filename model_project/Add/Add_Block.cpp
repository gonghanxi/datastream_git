#include "Add_Block.h"

Add_Block::Add_Block(const std::string& name)
	: Block(name)
{
}

void Add_Block::SetDefaultParamters()
{
}

bool Add_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);
    auto inputData = ReadInputData<double>(inputPort);

    if (inputData.empty()) {
        return false;
    }
    qDebug() << "Add_Block::Run-- inputData: " << inputData.size();

    double acc(0.0);
    for (size_t i = 0; i < inputData.size(); ++i) {
        acc += inputData[i];
    }

    std::vector<double> outputData;
    outputData.push_back(acc);
    qDebug() << "Add_Block::Run-- outputData: " << outputData[0];
    WriteOutputData(outputPort, outputData);

    return true;
}

bool Add_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    for(const auto& bridge_reader : bridge_readers) {
        std::vector<double> inputData;
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

    std::vector<double> outputData(1, 0.0);

    if(CanProcessData) {
        for(size_t i = 0; i < 1; ++i) {
            double sum = 0.0;


            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                sum += it->second[i];
            }

            outputData[i] = sum;
            //将处理结果放入输出队列
            m_outputQueue.push(outputData[i]);
        }
        //执行写入
        if (!m_outputQueue.empty()) {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<double>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[Add_Block] :" << m_outputCount
                     << " value:" << outputValue;
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

bool Add_Block::Setup()
{
	Block::Setup();
    // 清空输出队列
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
	return true;
}

bool Add_Block::Run()
{
    //时间驱动
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool Add_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_add = std::make_unique<Add>();

	m_add->output.SetRate(1U);

	AddInputPort("input", m_add->input, 1, Block::DataType::DOUBLE_BUS);
	AddOutputPort("output", m_add->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

	return true;
}
