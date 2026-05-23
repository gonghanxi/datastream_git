#include "ReverseCx_Block.h"

ReverseCx_Block::ReverseCx_Block(const std::string& name)
	: Block(name)
	, m_n(64)
{
}

void ReverseCx_Block::SetDefaultParamters()
{
	m_n = 64;
}

void ReverseCx_Block::SetParameters(int n)
{
	m_n = n;
	if (m_reverseCx) {
		m_reverseCx->N = n;
	}
}

bool ReverseCx_Block::Setup()
{
	Block::Setup();
	return true;
}

bool ReverseCx_Block::Run()
{
	if (!CanProcess()) {
		return false;
	}

	std::string inputPort = GetInputPortName(0);
	std::string outputPort = GetOutputPortName(0);

	auto inputData = ReadInputData<std::complex<double>>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<std::complex<double>> outputData;
	outputData.reserve(inputData.size());

	for (size_t i = 0; i < inputData.size(); ++i) {
		outputData.push_back(inputData[inputData.size() - 1 - i]);
	}

	WriteOutputData(outputPort, outputData);

	return true;
}

bool ReverseCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_reverseCx = std::make_unique<ReverseCx>();

	SetDefaultParamters();

	try { m_n = std::stoi(getParameter("N").Value); } catch (...) { }

	if (m_n <= 0) {
		std::cout << "Port rate must be greater than 0." << std::endl;
		return false;
	}

	SetParameters(m_n);

	AddInputPort("input", m_reverseCx->input, static_cast<size_t>(m_n), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
	AddOutputPort("output", m_reverseCx->output, static_cast<size_t>(m_n), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	return true;
}
