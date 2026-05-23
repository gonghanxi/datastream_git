#include "Sub_Block.h"

Sub_Block::Sub_Block(const std::string& name)
	: Block(name)
{
}

void Sub_Block::SetDefaultParamters()
{
}

bool Sub_Block::Setup()
{
	Block::Setup();
	return true;
}

bool Sub_Block::Run()
{
	if (!CanProcess()) {
		return false;
	}

	std::string posPort = GetInputPortName(0);
	std::string negPort = GetInputPortName(1);
	std::string outputPort = GetOutputPortName(0);

	auto posData = ReadInputData<double>(posPort);
	if (posData.empty()) {
		return true;
	}

	auto negData = ReadInputData<double>(negPort);

	double acc = posData[0];
	for (size_t i = 0; i < negData.size(); ++i) {
		acc -= negData[i];
	}

	std::vector<double> outputData;
	outputData.push_back(acc);
	WriteOutputData(outputPort, outputData);

	return true;
}

bool Sub_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

    m_sub = std::make_unique<Sub>();

	AddInputPort("pos", m_sub->pos, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddInputPort("neg", m_sub->neg, 1, Block::DataType::DOUBLE_BUS);
	AddOutputPort("output", m_sub->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

	return true;
}
