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
	return true;
}

bool ReverseInt_Block::Run()
{
	if (!CanProcess()) {
		return false;
	}

	std::string inputPort = GetInputPortName(0);
	std::string outputPort = GetOutputPortName(0);

	auto inputData = ReadInputData<int>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<int> outputData;
	outputData.reserve(inputData.size());

	for (size_t i = 0; i < inputData.size(); ++i) {
		outputData.push_back(inputData[inputData.size() - 1 - i]);
	}

	WriteOutputData(outputPort, outputData);

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
