#include "GainInt_Block.h"

GainInt_Block::GainInt_Block(const std::string& name)
	: Block(name)
	, m_gain(1.0)
{
}

void GainInt_Block::SetDefaultParamters()
{
	m_gain = 1.0;
}

void GainInt_Block::SetParameters(double gain)
{
	m_gain = gain;
	if (m_gainInt) {
		m_gainInt->m_Gain = gain;
	}
}

bool GainInt_Block::Setup()
{
	Block::Setup();
	return true;
}

bool GainInt_Block::Run()
{

	std::string inputPort = GetInputPortName(0);
	auto inputData = ReadInputData<int>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<int> outputData;
	outputData.reserve(inputData.size());

	for (int x : inputData) {
		outputData.push_back(static_cast<int>(m_gain * x));
	}

	WriteOutputData(GetOutputPortName(0), outputData);
	return true;
}

bool GainInt_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_gainInt = std::make_unique<GainInt>();

	AddInputPort("input", m_gainInt->input, 1, Block::DataType::CIRCULAR_BUFFER_INT);
	AddOutputPort("output", m_gainInt->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

	SetDefaultParamters();

	try { m_gain = std::stod(getParameter("m_Gain").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'm_Gain', using default value."); }

	SetParameters(m_gain);

	return true;
}
