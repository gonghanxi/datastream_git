#include "ReverseEnv_Block.h"

ReverseEnv_Block::ReverseEnv_Block(const std::string& name)
	: Block(name)
	, m_n(64)
{
}

void ReverseEnv_Block::SetDefaultParamters()
{
	m_n = 64;
}

void ReverseEnv_Block::SetParameters(int n)
{
	m_n = n;
	if (m_reverseEnv) {
		m_reverseEnv->N = n;
	}
}

bool ReverseEnv_Block::Setup()
{
	Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool ReverseEnv_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool ReverseEnv_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        outputData.push_back(inputData[inputData.size() - 1 - i]);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool ReverseEnv_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_n)) {
        std::vector<EnvelopeSignal> outputData;
        outputData.reserve(m_inputBuffer.size());

        for (size_t i = 0; i < m_inputBuffer.size(); ++i) {
            outputData.push_back(m_inputBuffer[m_inputBuffer.size() - 1 - i]);
        }

        for(const auto& val : outputData) m_outputQueue.push(val);
        if (!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[ReverseEnv_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}

bool ReverseEnv_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_reverseEnv = std::make_unique<ReverseEnv>();

	SetDefaultParamters();

	try { m_n = std::stoi(getParameter("N").Value); } catch (...) { }

	if (m_n <= 0) {
		std::cout << "Port rate must be greater than 0." << std::endl;
		return false;
	}

	SetParameters(m_n);

	AddInputPort("input", m_reverseEnv->input, static_cast<size_t>(m_n), Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("output", m_reverseEnv->output, static_cast<size_t>(m_n), Block::DataType::ENVELOPE_SIGNAL);

	return true;
}
