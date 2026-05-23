#include "Mpy_Block.h"

Mpy_Block::Mpy_Block(const std::string& name)
	: Block(name)
{
}

void Mpy_Block::SetDefaultParamters()
{
}

bool Mpy_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    double acc = 1.0;
    for (size_t i = 0; i < inputData.size(); ++i) {
        acc *= inputData[i];
    }

    std::vector<double> outputData;
    outputData.push_back(acc);

    WriteOutputData(outputPort, outputData);

    return true;
}

bool Mpy_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();

    for(const auto& bridge_reader : bridge_readers) {
        std::vector<double> inputData;
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

    std::vector<double> outputData(1);  // 初始化为0

    if(CanProcessData) {
        // 遍历每个位置
        for(size_t i = 0; i < 1; ++i) {
            double acc = 1.0;


            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                acc *= it->second[i];
            }

            outputData[i] = acc;
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

            qDebug() << "[Mpy_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

bool Mpy_Block::Setup()
{
	Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool Mpy_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Mpy_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

    m_mpy = std::make_unique<Mpy>();

	AddInputPort("input", m_mpy->input, 1, Block::DataType::DOUBLE_BUS);
	AddOutputPort("output", m_mpy->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

	return true;
}
