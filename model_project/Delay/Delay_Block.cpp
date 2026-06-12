#include "Delay_Block.h"
#include <algorithm>

Delay_Block::Delay_Block(const std::string& name)
	: Block(name)
{
}

void Delay_Block::SetDefaultParamters()
{
	m_n = 1;
	m_outputTiming = Delay::EqualToInput;
	m_head = 0;
	m_warmup = 0;
}

void Delay_Block::SetParameters(int n, Delay::OutputTimingEnum timing)
{
	m_n = n;
	m_outputTiming = timing;
	if (m_delay) {
		m_delay->N = n;
		m_delay->OutputTiming = timing;
	}
}

void Delay_Block::ResetState()
{
	m_buf.clear();
	m_head = 0;
	m_warmup = 0;

	if (m_n > 0) {
		m_buf.assign(static_cast<std::size_t>(m_n), 0.0);
		if (m_outputTiming == Delay::BeforeInput) {
			m_warmup = m_n;
		}
    }
}

bool Delay_Block::Setup()
{
	Block::Setup();
	return true;
}

bool Delay_Block::Run()
{
	std::string inputPort = GetInputPortName(0);
	std::string outputPort = GetOutputPortName(0);

	auto inputData = ReadInputData<double>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<double> outputData;
	outputData.reserve(inputData.size());

	if (m_n == 0) {
		for (size_t i = 0; i < inputData.size(); ++i) {
			outputData.push_back(inputData[i]);
		}
	} else {
	for (size_t i = 0; i < inputData.size(); ++i) {
			if (m_outputTiming == Delay::BeforeInput && m_warmup > 0) {
				outputData.push_back(0.0);
				// Even during warmup, advance the delay line with input samples.
				if (!m_buf.empty()) {
					m_buf[m_head] = inputData[i];
					m_head = (m_head + 1) % m_buf.size();
				}
				--m_warmup;
				continue;
			}
			outputData.push_back(m_buf[m_head]);
			m_buf[m_head] = inputData[i];
			m_head = (m_head + 1) % m_buf.size();
		}
	}

	WriteOutputData(outputPort, outputData);

	return true;
}

bool Delay_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_delay = std::make_unique<Delay>();

	m_delay->input.SetRate(1U);
	m_delay->output.SetRate(1U);

	AddInputPort("input", m_delay->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("output", m_delay->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

	try { m_n = std::stoi(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }
	try { m_outputTiming = ConvertStringToOutputTimingEnum(getParameter("OutputTiming").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputTiming', using default value."); }

	SetParameters(m_n, m_outputTiming);
	ResetState();

	return true;
}

Delay::OutputTimingEnum Delay_Block::ConvertStringToOutputTimingEnum(const std::string& value)
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

	if (lowerValue == "equaltoinput" || lowerValue == "0") {
		return Delay::EqualToInput;
	} else if (lowerValue == "beforeinput" || lowerValue == "1") {
		return Delay::BeforeInput;
	}
	return Delay::EqualToInput;
}
