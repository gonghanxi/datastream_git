#include "ReverseInt_Block.h"

ReverseInt_Block::ReverseInt_Block(const std::string& name)
	: Block(name)
	, m_n(64)
{
}

void ReverseInt_Block::SetDefaultParamters()
{
	m_n = 64;
}

void ReverseInt_Block::SetParameters(int n)
{
	m_n = n;
	if (m_reverseInt) {
		m_reverseInt->N = n;
	}
}

bool ReverseInt_Block::Setup()
{
	Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool ReverseInt_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool ReverseInt_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<int> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        outputData.push_back(inputData[inputData.size() - 1 - i]);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool ReverseInt_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_n)) {
        std::vector<int> outputData;
        outputData.reserve(m_inputBuffer.size());

        for (size_t i = 0; i < m_inputBuffer.size(); ++i) {
            outputData.push_back(m_inputBuffer[m_inputBuffer.size() - 1 - i]);
        }

        for(const auto& val : outputData) m_outputQueue.push(val);
        if (!m_outputQueue.empty()) {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<int>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[ReverseInt_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}

bool ReverseInt_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_reverseInt = std::make_unique<ReverseInt>();

	SetDefaultParamters();

	try { m_n = std::stoi(getParameter("N").Value); } catch (...) { }

	if (m_n <= 0) {
		std::cout << "Port rate must be greater than 0." << std::endl;
		return false;
	}

	SetParameters(m_n);

	AddInputPort("input", m_reverseInt->input, static_cast<size_t>(m_n), Block::DataType::CIRCULAR_BUFFER_INT);
	AddOutputPort("output", m_reverseInt->output, static_cast<size_t>(m_n), Block::DataType::CIRCULAR_BUFFER_INT);

	return true;
}
