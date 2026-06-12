#include "DownSampleCx_Block.h"
#include <algorithm>
#include <complex>
#include <vector>

DownSampleCx_Block::DownSampleCx_Block(const std::string& name)
	: Block(name)
{
}

void DownSampleCx_Block::SetDefaultParamters()
{
	m_factor = 2;
	m_phase = 0;
}

void DownSampleCx_Block::SetParameters()
{
	if (!m_downSampleCx) {
		return;
	}

	m_downSampleCx->Factor = m_factor;
    m_downSampleCx->Phase = m_phase;
}

bool DownSampleCx_Block::Setup()
{
	Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool DownSampleCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool DownSampleCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_downSampleCx = std::make_unique<DownSampleCx>();



	SetDefaultParamters();

	try { m_factor = std::stoi(getParameter("Factor").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Factor', using default value."); }
	try { m_phase = std::stoi(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }

	SetParameters();

    if(!m_downSampleCx->Setup()) return false;

    AddInputPort("input", m_downSampleCx->input, m_factor, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_downSampleCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	return true;
}

bool DownSampleCx_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(inputData[static_cast<size_t>(m_phase)]);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool DownSampleCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_factor)) {
        m_outputQueue.push(m_inputBuffer[static_cast<size_t>(m_phase)]);
        if(!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[DownSampleCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}
