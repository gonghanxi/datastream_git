#include "GainCx_Block.h"

GainCx_Block::GainCx_Block(const std::string& name)
	: Block(name)
	, m_gain(1.0)
{
}

void GainCx_Block::SetDefaultParamters()
{
	m_gain = 1.0;
}

void GainCx_Block::SetParameters(double gain)
{
	m_gain = gain;
	if (m_gainCx) {
		m_gainCx->m_Gain = gain;
	}
}

bool GainCx_Block::Setup()
{
	Block::Setup();
	return true;
}

bool GainCx_Block::Run()
{
	std::string inputPort = GetInputPortName(0);
	auto inputData = ReadInputData<std::complex<double>>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<std::complex<double>> outputData;
	outputData.reserve(inputData.size());

	for (const auto& x : inputData) {
		outputData.push_back(m_gain * x);
	}

	WriteOutputData(GetOutputPortName(0), outputData);
	return true;
}

bool GainCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_gainCx = std::make_unique<GainCx>();

	AddInputPort("input", m_gainCx->input, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
	AddOutputPort("output", m_gainCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	SetDefaultParamters();

	try { m_gain = std::stod(getParameter("m_Gain").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'm_Gain', using default value."); }

	SetParameters(m_gain);

	return true;
}
