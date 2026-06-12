#include "BlockAllPole_Block.h"

BlockAllPole_Block::BlockAllPole_Block(const std::string& name)
	: Block(name)
	, m_blockSize(128)
	, m_order(16)
{
}

void BlockAllPole_Block::SetDefaultParamters()
{
	m_blockSize = 128;
	m_order = 16;
}

bool BlockAllPole_Block::ValidateParameters()
{
	if (m_blockSize < 1) {
        LOG_ERROR("BlockSize must be > 0.");
		return false;
	}
	if (m_order < 1) {
        LOG_ERROR("Order must be > 0.");
		return false;
	}

	if (m_taps.size() != static_cast<std::size_t>(m_order)) {
		m_taps.assign(static_cast<std::size_t>(m_order), 0.0);
	}

	if (m_delayLine.size() != static_cast<std::size_t>(m_order)) {
		m_delayLine.assign(static_cast<std::size_t>(m_order), 0.0);
	}

    return true;
}

bool BlockAllPole_Block::DataStreamRun()
{
    if (!ValidateParameters()) {
        return false;
    }

    const std::string inputPort = GetInputPortName(0);
    const std::string coefPort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto coefData = ReadInputData<double>(coefPort);

    if (inputData.empty() || coefData.empty()) {
        return true;
    }

    const std::size_t order = static_cast<std::size_t>(m_order);
    const std::size_t blockSize = static_cast<std::size_t>(m_blockSize);

    for (std::size_t k = 0; k < order; ++k) {
        m_taps[k] = coefData[k];
    }

    std::vector<double> outputData;
    outputData.reserve(blockSize);

    for (std::size_t n = 0; n < blockSize; ++n) {
        const double x = inputData[n];

        double y = x;
        for (std::size_t k = 0; k < order; ++k) {
            y += m_taps[k] * m_delayLine[k];
        }

        outputData.push_back(y);

        for (std::size_t k = order; k > 1; --k) {
            m_delayLine[k - 1] = m_delayLine[k - 2];
        }
        m_delayLine[0] = y;
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool BlockAllPole_Block::TimeDrivenRun()
{
    if (!ValidateParameters()) {
        return false;
    }

    const std::string inputPort = GetInputPortName(0);
    const std::string coefPort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto coefData = ReadInputData<double>(coefPort);

    if (inputData.empty() || coefData.empty()) {
        return true;
    }
    for(size_t i = 0; i < inputData.size(); i++) {
        m_signalInBuffer.push_back(inputData[i]);
    }
    for(size_t i = 0; i < coefData.size(); i++) {
        m_coefsInBuffer.push_back(coefData[i]);
    }
    if(m_signalInBuffer.size() >= static_cast<size_t>(m_blockSize) && m_coefsInBuffer.size() >= static_cast<size_t>(m_order)) {
        const std::size_t order = static_cast<std::size_t>(m_order);
        const std::size_t blockSize = static_cast<std::size_t>(m_blockSize);

        for (std::size_t k = 0; k < order; ++k) {
            m_taps[k] = coefData[k];
        }

        std::vector<double> outputData;
        outputData.reserve(blockSize);

        for (std::size_t n = 0; n < blockSize; ++n) {
            const double x = inputData[n];

            double y = x;
            for (std::size_t k = 0; k < order; ++k) {
                y += m_taps[k] * m_delayLine[k];
            }

            outputData.push_back(y);

            for (std::size_t k = order; k > 1; --k) {
                m_delayLine[k - 1] = m_delayLine[k - 2];
            }
            m_delayLine[0] = y;
        }
        for (const auto& val : outputData)
            m_outputQueue.push(val);
        m_signalInBuffer.clear();
        m_coefsInBuffer.clear();
        if (!m_outputQueue.empty())
        {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}

bool BlockAllPole_Block::Setup()
{
	Block::Setup();
    if(!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool BlockAllPole_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BlockAllPole_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_blockAllPole = std::make_unique<SystemVueModelBuilder::BlockAllPole>();

	SetDefaultParamters();

	try { m_blockSize = std::stoi(getParameter("BlockSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BlockSize', using default value."); }
	try { m_order = std::stoi(getParameter("Order").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Order', using default value."); }

	if (!ValidateParameters()) {
		return false;
	}

	AddInputPort("signalIn", m_blockAllPole->m_signalIn, static_cast<size_t>(m_blockSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddInputPort("coefs", m_blockAllPole->m_coefs, static_cast<size_t>(m_order), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("signalOut", m_blockAllPole->m_signalOut, static_cast<size_t>(m_blockSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	return true;
}



