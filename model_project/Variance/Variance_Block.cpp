#include "Variance_Block.h"

Variance_Block::Variance_Block(const std::string& name)
	: Block(name)
{
}

void Variance_Block::SetDefaultParamters()
{
	m_blockSize = 1;
	m_sum = 0.0;
	m_sumSqr = 0.0;
	m_sumN = 0;
}

void Variance_Block::SetParameters(int blockSize)
{
	m_blockSize = blockSize;
	if (m_variance) {
		m_variance->BlockSize = blockSize;
    }
}

bool Variance_Block::DataStreamRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string meanPort = GetOutputPortName(0);
    const std::string varPort = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    const std::size_t n = std::min<std::size_t>(inputData.size(), static_cast<std::size_t>(std::max(1, m_blockSize)));
    m_sumN += static_cast<int>(n);

    for (std::size_t i = 0; i < n; ++i) {
        m_sum += inputData[i];
        m_sumSqr += inputData[i] * inputData[i];
    }

    const double meanVal = (m_sumN > 0) ? (m_sum / m_sumN) : 0.0;
    const double varVal = (m_sumN > 0) ? (m_sumSqr / m_sumN - meanVal * meanVal) : 0.0;

    WriteOutputData(meanPort, std::vector<double>{meanVal});
    WriteOutputData(varPort, std::vector<double>{varVal});

    return true;
}

bool Variance_Block::TimeDrivenRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string meanPort = GetOutputPortName(0);
    const std::string varPort = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inBuffer.push_back(val);

    if(m_inBuffer.size() >= static_cast<std::size_t>(m_blockSize)) {
        const std::size_t n = std::min<std::size_t>(m_inBuffer.size(), static_cast<std::size_t>(std::max(1, m_blockSize)));
        m_sumN += static_cast<int>(n);

        for (std::size_t i = 0; i < n; ++i) {
            m_sum += m_inBuffer[i];
            m_sumSqr += m_inBuffer[i] * m_inBuffer[i];
        }

        const double meanVal = (m_sumN > 0) ? (m_sum / m_sumN) : 0.0;
        const double varVal = (m_sumN > 0) ? (m_sumSqr / m_sumN - meanVal * meanVal) : 0.0;

        m_meanQueue.push(meanVal);
        m_varianceQueue.push(varVal);
        m_inBuffer.clear();
    }
    if(!m_meanQueue.empty() && m_varianceQueue.empty()) {
        double meanValue = m_meanQueue.front();
        m_meanQueue.pop();
        double varianceValue = m_varianceQueue.front();
        m_varianceQueue.pop();
        m_outputCount++;

        WriteOutputData(meanPort, std::vector<double>{meanValue});
        WriteOutputData(varPort, std::vector<double>{varianceValue});
        m_lastmean = meanValue;
        m_lastvariance = varianceValue;

        qDebug() << "[Variance_Block] 分发输出:" << m_outputCount
                 << " value:" << meanValue << "|" << varianceValue;
    }
    return true;
}

bool Variance_Block::Setup()
{
	Block::Setup();
    while(!m_meanQueue.empty()) m_meanQueue.pop();
    while(!m_varianceQueue.empty()) m_varianceQueue.pop();
	return true;
}

bool Variance_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Variance_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_variance = std::make_unique<Variance>();

	SetDefaultParamters();

	try { m_blockSize = std::stoi(getParameter("BlockSize").Value); } catch (...) { }

	if (m_blockSize <= 0) {
		std::cout << "BlockSize must be greater than 0." << std::endl;
		return false;
	}

	SetParameters(m_blockSize);

	AddInputPort("in", m_variance->in, static_cast<std::size_t>(m_blockSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("mean", m_variance->mean, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("variance", m_variance->variance, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	return true;
}
