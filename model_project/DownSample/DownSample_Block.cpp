#include "DownSample_Block.h"

DownSample_Block::DownSample_Block(const std::string& name)
	: Block(name)
{
}

void DownSample_Block::SetDefaultParamters()
{
	m_factor = 2;
	m_phase = 0;
}

void DownSample_Block::SetParameters(int factor, int phase)
{
	m_factor = factor;
	m_phase = phase;
	if (m_downSample) {
		m_downSample->Factor = factor;
		m_downSample->Phase = phase;
    }
}

bool DownSample_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(inputData[static_cast<size_t>(m_phase)]);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool DownSample_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_factor)) {
        m_outputQueue.push(m_inputBuffer[static_cast<size_t>(m_phase)]);
        if(!m_outputQueue.empty()) {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[DownSample_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}

bool DownSample_Block::Setup()
{
	Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool DownSample_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool DownSample_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_downSample = std::make_unique<DownSample>();

	SetDefaultParamters();

	try { m_factor = std::stoi(getParameter("Factor").Value); } catch (...) {}
	try { m_phase = std::stoi(getParameter("Phase").Value); } catch (...) {}

    SetParameters(m_factor, m_phase);

    if(!m_downSample->Setup()) return false;

    AddInputPort("input", m_downSample->input, m_factor, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_downSample->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	return true;
}
