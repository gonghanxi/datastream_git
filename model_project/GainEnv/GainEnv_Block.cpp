#include "GainEnv_Block.h"

GainEnv_Block::GainEnv_Block(const std::string& name)
	: Block(name)
	, m_gain(1.0)
{
}

void GainEnv_Block::SetDefaultParamters()
{
	m_gain = 1.0;
}

void GainEnv_Block::SetParameters(double gain)
{
	m_gain = gain;
	if (m_gainEnv) {
		m_gainEnv->m_Gain = gain;
	}
}

bool GainEnv_Block::Setup()
{
	Block::Setup();
	return true;
}

bool GainEnv_Block::Run()
{
	std::string inputPort = GetInputPortName(0);
	auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
	outputData.reserve(inputData.size());

	for (const auto& x : inputData) {
		outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(m_gain * x.complex()));
	}

	WriteOutputData(GetOutputPortName(0), outputData);
	return true;
}

bool GainEnv_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_gainEnv = std::make_unique<GainEnv>();

	AddInputPort("input", m_gainEnv->input, 1, Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("output", m_gainEnv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

	SetDefaultParamters();

	try { m_gain = std::stod(getParameter("m_Gain").Value); } catch (...) { }

	SetParameters(m_gain);

	return true;
}
