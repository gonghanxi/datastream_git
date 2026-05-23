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
	return true;
}

bool ReverseEnv_Block::Run()
{
	if (!CanProcess()) {
		return false;
	}

	std::string inputPort = GetInputPortName(0);
	std::string outputPort = GetOutputPortName(0);

	auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
	outputData.reserve(inputData.size());

	for (size_t i = 0; i < inputData.size(); ++i) {
		outputData.push_back(inputData[inputData.size() - 1 - i]);
	}

	WriteOutputData(outputPort, outputData);

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
