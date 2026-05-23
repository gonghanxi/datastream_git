#include "DownSampleEnv_Block.h"

DownSampleEnv_Block::DownSampleEnv_Block(const std::string& name)
	: Block(name)
{
}

void DownSampleEnv_Block::SetDefaultParamters()
{
	m_factor = 2;
	m_phase = 0;
}

void DownSampleEnv_Block::SetParameters(int factor, int phase)
{
	m_factor = factor;
	m_phase = phase;
	if (m_downSampleEnv) {
		m_downSampleEnv->Factor = factor;
		m_downSampleEnv->Phase = phase;
	}
}

bool DownSampleEnv_Block::Setup()
{
	Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool DownSampleEnv_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool DownSampleEnv_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_downSampleEnv = std::make_unique<DownSampleEnv>();

	SetDefaultParamters();

	try { m_factor = std::stoi(getParameter("Factor").Value); } catch (...) {}
	try { m_phase = std::stoi(getParameter("Phase").Value); } catch (...) {}

    if(!m_downSampleEnv->Setup()) return false;

	AddInputPort("input", m_downSampleEnv->input, m_factor, Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("output", m_downSampleEnv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

void DownSampleEnv_Block::UpdateCharacterizationFrequency()
{
	if (m_downSampleEnv) {
		m_downSampleEnv->PropagateCharacterizationFrequency();
		GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(
			GetInputPort(GetInputPortName(0))->getCharacterizationFrequency());
	}
}

bool DownSampleEnv_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    UpdateCharacterizationFrequency();

    std::vector<EnvelopeSignal> outputData;
    outputData.push_back(inputData[static_cast<size_t>(m_phase)]);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool DownSampleEnv_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_factor)) {
        m_outputQueue.push(m_inputBuffer[static_cast<size_t>(m_phase)]);
        if(!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[DownSampleCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}








