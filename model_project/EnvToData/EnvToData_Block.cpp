#include "EnvToData_Block.h"

EnvToData_Block::EnvToData_Block(const std::string& name)
	: Block(name)
{
}

void EnvToData_Block::SetDefaultParamters()
{
}

bool EnvToData_Block::Setup()
{
	Block::Setup();
    return true;
}

bool EnvToData_Block::Run()
{

	std::string inputPort = GetInputPortName(0);
	auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
	if (inputData.empty()) {
    return true;
    }
	UpdateCharacterizationFrequency();

    Buffer* fc = GetOutputPort(GetOutputPortName(0));
    Buffer* time = GetOutputPort(GetOutputPortName(1));
    Buffer* I = GetOutputPort(GetOutputPortName(2));
    Buffer* Q = GetOutputPort(GetOutputPortName(3));

	std::vector<double> outFc;
	std::vector<double> outTime;
	std::vector<double> outI;
	std::vector<double> outQ;
	outFc.reserve(inputData.size());
	outTime.reserve(inputData.size());
	outI.reserve(inputData.size());
	outQ.reserve(inputData.size());

	const SimuParameter simulator_param = getSimu();
	const double fs = simulator_param.samplingRate;
	const double baseCount = static_cast<double>(m_envToData->GetCount());

	for (size_t i = 0; i < inputData.size(); ++i) {
		const auto& sig = inputData[i];
		outI.push_back(sig.real());
		outQ.push_back(sig.imag());
		outTime.push_back((fs > 0.0)
			? (simulator_param.startTime + (baseCount + static_cast<double>(i)) / fs)
			: 0.0);
		outFc.push_back(GetInputPort(GetInputPortName(0))->getCharacterizationFrequency()); // TODO: input not connected; value may be unreliable
	}

    if(fc->GetReaderCount() != 0) {
        WriteOutputData(GetOutputPortName(0), outFc);
    }
    if(time->GetReaderCount() != 0) {
        WriteOutputData(GetOutputPortName(1), outTime);
    }
    if(I->GetReaderCount() != 0) {
        WriteOutputData(GetOutputPortName(2), outI);
    }
    if(Q->GetReaderCount() != 0) {
        WriteOutputData(GetOutputPortName(3), outQ);
    }

	if (m_envToData) {
		m_envToData->Advance();
	}
    return true;
}

bool EnvToData_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_envToData = std::make_unique<EnvToData>();

	AddInputPort("input", m_envToData->input, 1, Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("fc", m_envToData->fc, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("time", m_envToData->time, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("I", m_envToData->I, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("Q", m_envToData->Q, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

    return true;
}

void EnvToData_Block::UpdateCharacterizationFrequency()
{
    std::string inputPort = GetInputPortName(0);
}









