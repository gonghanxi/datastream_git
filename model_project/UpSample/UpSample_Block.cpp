#include "UpSample_Block.h"
#include <algorithm>

UpSample_Block::UpSample_Block(const std::string& name)
	: Block(name)
{
}

void UpSample_Block::SetDefaultParamters()
{
	m_factor = 2;
	m_mode = UpSample::Insertzeros;
	m_phase = 0;
}

void UpSample_Block::SetParameters(int factor, UpSample::ModeEnum mode, int phase)
{
	m_factor = factor;
	m_phase = phase;
	m_mode = mode;
	if (m_upSample) {
		m_upSample->Factor = m_factor;
		m_upSample->Phase = m_phase;
		m_upSample->Mode = m_mode;
    }
}

bool UpSample_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return false;
    }

    size_t inLen = inputData.size();
    size_t outLen = inLen * static_cast<size_t>(std::max(1, m_factor));
    std::vector<double> outputData;
    outputData.reserve(outLen);

    if (m_mode == UpSample::Insertzeros) {
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

bool UpSample_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= 1) {
        size_t inLen = m_inputBuffer.size();
        size_t outLen = inLen * static_cast<size_t>(std::max(1, m_factor));
        std::vector<double> outputData;
        outputData.reserve(outLen);

        if (m_mode == UpSample::Insertzeros) {
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
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[UpSample_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
            m_inputBuffer.clear();
        }
    }
    return true;
}

bool UpSample_Block::Setup()
{
	Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool UpSample_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool UpSample_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_upSample = std::make_unique<UpSample>();

	AddInputPort("input", m_upSample->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("output", m_upSample->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

	try { m_factor = std::stoi(getParameter("Factor").Value); } catch (...) { }
	try { m_phase = std::stoi(getParameter("Phase").Value); } catch (...) { }
	try { m_mode = ConvertStringToModeEnum(getParameter("Mode").Value); } catch (...) { }

    SetParameters(m_factor, m_mode, m_phase);

	GetOutputPort(GetOutputPortName(0))->SetWriteSize(m_factor);

	return true;
}

UpSample::ModeEnum UpSample_Block::ConvertStringToModeEnum(const std::string& value)
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
		[](unsigned char c) { return std::tolower(c); });

	if (lowerValue == "insertzeros" || lowerValue == "0") {
		return UpSample::Insertzeros;
	} else if (lowerValue == "holdsample" || lowerValue == "1") {
		return UpSample::Holdsample;
	}
	return UpSample::Insertzeros;
}
