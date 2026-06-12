#include "SetSampleRateInt_Block.h"
#include <cmath>

namespace {
bool almost_equal(double a, double b, double eps = 1e-12)
{
	const double ma = std::fabs(a), mb = std::fabs(b);
	const double scale = (ma > mb ? ma : mb);
	return std::fabs(a - b) <= eps * (scale > 1.0 ? scale : 1.0);
}
}

SetSampleRateInt_Block::SetSampleRateInt_Block(const std::string& name)
	: Block(name)
{
}

void SetSampleRateInt_Block::SetDefaultParamters()
{
	m_sampleRate = getSimu().samplingRate;
}

void SetSampleRateInt_Block::SetParameters(double sampleRate)
{
	m_sampleRate = sampleRate;
	if (m_setSampleRate) {
		m_setSampleRate->SampleRate = sampleRate;
	}
}

bool SetSampleRateInt_Block::Setup()
{
	Block::Setup();
	return true;
}

bool SetSampleRateInt_Block::Run()
{
	if (!CanProcess()) {
		return false;
	}

	std::string inputPort = GetInputPortName(0);
	std::string outputPort = GetOutputPortName(0);

	auto inputData = ReadInputData<int>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	if (!ValidateSampleRate()) {
		return false;
	}

	std::vector<int> outputData;
	outputData.reserve(inputData.size());
	for (const auto& v : inputData) {
		outputData.push_back(v);
        m_setSampleRate->Advance();
	}

	WriteOutputData(outputPort, outputData);

	return true;
}

bool SetSampleRateInt_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_setSampleRate = std::make_unique<SetSampleRateInt>();

	AddInputPort("input", m_setSampleRate->input, 1, Block::DataType::TIMED_INT);
	AddOutputPort("output", m_setSampleRate->output, 1, Block::DataType::TIMED_INT);

	SetDefaultParamters();
	try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    SetParameters(m_sampleRate);

	return true;
}

bool SetSampleRateInt_Block::ValidateSampleRate()
{
	if (!(m_sampleRate > 0.0)) {
		std::cout << "SetSampleRate_self: Please enter a positive SampleRate (e.g., 1e6). "
				  << "The default placeholder 'Sample_Rate' is not a real value." << std::endl;
		return false;
	}

	const double fsIn = m_setSampleRate->input.GetSampleRate();
	if (fsIn > 0.0 && !almost_equal(fsIn, m_sampleRate)) {
		std::cout << "SetSampleRate_self: input is already timed at "
				  << fsIn << " Hz, but SampleRate = " << m_sampleRate
				  << " Hz. Use a resampler to change rate, or match SampleRate."
				  << std::endl;
		return false;
	}

        m_setSampleRate->output.SetSampleRate(m_sampleRate);
	return true;
}




