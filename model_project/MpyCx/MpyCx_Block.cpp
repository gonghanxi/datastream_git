#include "MpyCx_Block.h"

MpyCx_Block::MpyCx_Block(const std::string& name)
	: Block(name)
{
}

void MpyCx_Block::SetDefaultParamters()
{
}

bool MpyCx_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    std::complex<double> acc(1.0, 0.0);
    for (const auto& v : inputData) {
        acc *= v;
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(acc);

    WriteOutputData(outputPort, outputData);

    return true;
}

bool MpyCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();

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
            std::complex<double> acc(1.0,0.0);


            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                acc *= it->second[i];
            }

            outputData[i] = acc;
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

bool MpyCx_Block::Setup()
{
	Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool MpyCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool MpyCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_mpyCx = std::make_unique<MpyCx>();

	AddInputPort("input", m_mpyCx->input, 1, Block::DataType::DCOMPLEX_BUS);
	AddOutputPort("output", m_mpyCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	SetDefaultParamters();

	return true;
}
