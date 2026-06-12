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

bool ReverseCx_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        outputData.push_back(inputData[inputData.size() - 1 - i]);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool ReverseCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_n)) {
        std::vector<std::complex<double>> outputData;
        outputData.reserve(m_inputBuffer.size());

        for (size_t i = 0; i < m_inputBuffer.size(); ++i) {
            outputData.push_back(m_inputBuffer[m_inputBuffer.size() - 1 - i]);
        }

        for(const auto& val : outputData) m_outputQueue.push(val);
        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[ReverseCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            return true;
        }
    }
    return true;
}

bool ReverseCx_Block::Setup()
{
	Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool ReverseCx_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool ReverseCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_reverseCx = std::make_unique<ReverseCx>();

	SetDefaultParamters();

	try { m_n = std::stoi(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }

	if (m_n <= 0) {
        LOG_ERROR("Port rate must be greater than 0.");
		return false;
	}

	SetParameters(m_n);

	AddInputPort("input", m_reverseCx->input, static_cast<size_t>(m_n), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
	AddOutputPort("output", m_reverseCx->output, static_cast<size_t>(m_n), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	return true;
}
