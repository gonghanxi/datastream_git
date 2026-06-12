#include "UpSampleCx_Block.h"
#include <algorithm>
#include <cctype>

UpSampleCx_Block::UpSampleCx_Block(const std::string& name)
	: Block(name),
	m_isInRun(false)
{
}

void UpSampleCx_Block::SetDefaultParamters()
{
	m_factor = 2;
	m_mode = UpSampleCx::Insertzeros;
	m_phase = 0;
}

void UpSampleCx_Block::SetParameters(int factor, UpSampleCx::ModeEnum mode, int phase)
{
	m_factor = factor;
	m_phase = phase;
	m_mode = mode;
	if (m_upSampleCx) {
		m_upSampleCx->Factor = m_factor;
		m_upSampleCx->Phase = m_phase;
		m_upSampleCx->Mode = m_mode;
	}
}

bool UpSampleCx_Block::ValidatePhase() const
{
	if (m_mode != UpSampleCx::Insertzeros) {
		return true;
	}
	if (m_factor <= 0) {
		return false;
	}
	if (m_phase < 0 || m_phase >= m_factor) {
		return false;
	}
	return true;
}

bool UpSampleCx_Block::Setup()
{
	Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool UpSampleCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool UpSampleCx_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if (inputData.empty()) {
        return false;
    }

    size_t inLen = inputData.size();
    size_t outLen = inLen * static_cast<size_t>(std::max(1, m_factor));
    std::vector<std::complex<double>> outputData;
    outputData.reserve(outLen);

    if (m_mode == UpSampleCx::Insertzeros) {
        for (size_t i = 0; i < inLen; ++i) {
            // Fill Factor samples with zeros
            for (int j = 0; j < m_factor; ++j) {
                outputData.push_back(0.0);
            }
            // Place input sample at Phase (if valid)
            if (m_phase >= 0 && m_phase < m_factor) {
                const size_t base = i * static_cast<size_t>(m_factor);
                outputData[base + static_cast<size_t>(m_phase)] = inputData[i];
            }
        }
    } else {
        for (size_t i = 0; i < inLen; ++i) {
            for (int j = 0; j < m_factor; ++j) {
                outputData.push_back(inputData[i]);
            }
        }
    }

    WriteOutputData(outputPortName, outputData);

    return true;
}

bool UpSampleCx_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= 1) {
        size_t inLen = m_inputBuffer.size();
        size_t outLen = inLen * static_cast<size_t>(std::max(1, m_factor));
        std::vector<std::complex<double>> outputData;
        outputData.reserve(outLen);

        if (m_mode == UpSampleCx::Insertzeros) {
            for (size_t i = 0; i < inLen; ++i) {
                // Fill Factor samples with zeros
                for (int j = 0; j < m_factor; ++j) {
                    outputData.push_back(0.0);
                }
                // Place input sample at Phase (if valid)
                if (m_phase >= 0 && m_phase < m_factor) {
                    const size_t base = i * static_cast<size_t>(m_factor);
                    outputData[base + static_cast<size_t>(m_phase)] = m_inputBuffer[i];
                }
            }
        } else {
            for (size_t i = 0; i < inLen; ++i) {
                for (int j = 0; j < m_factor; ++j) {
                    outputData.push_back(m_inputBuffer[i]);
                }
            }
        }

        for(const auto& val : outputData) m_outputQueue.push(val);

        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[UpSampleCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
        }
    }
    return true;
}

bool UpSampleCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_upSampleCx = std::make_unique<UpSampleCx>();

	AddInputPort("input", m_upSampleCx->input, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
	AddOutputPort("output", m_upSampleCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	SetDefaultParamters();

	try { m_factor = std::stoi(getParameter("Factor").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Factor', using default value."); }
	try { m_phase = std::stoi(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }
	try { m_mode = ConvertStringToModeEnum(getParameter("Mode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Mode', using default value."); }

	SetParameters(m_factor, m_mode, m_phase);

	if (m_factor < 1) {
		std::cout << "UpSampleCx: Factor must be >= 1." << std::endl;
		return false;
	}

	if (!ValidatePhase()) {
		std::cout << "UpSampleCx: Phase must be >= 0 and < Factor." << std::endl;
		return false;
	}

	return true;
}

UpSampleCx::ModeEnum UpSampleCx_Block::ConvertStringToModeEnum(const std::string& value)
{
	std::string trimmedValue;
	trimmedValue.reserve(value.size());
	for (char c : value) {
		if (!std::isspace(static_cast<unsigned char>(c))) {
			trimmedValue.push_back(c);
		}
	}
	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (lowerValue == "insertzeros" || lowerValue == "0") {
		return UpSampleCx::Insertzeros;
	} else if (lowerValue == "holdsample" || lowerValue == "1") {
		return UpSampleCx::Holdsample;
	}
	return UpSampleCx::Insertzeros;
}
