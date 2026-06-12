#include "ConstCx_Block.h"
#include <algorithm>
#include <cctype>

namespace {
std::string TrimCopy(const std::string& value)
{
	std::string s = value;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
	return s;
}

std::string ToLowerCopy(const std::string& value)
{
	std::string s = value;
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return s;
}
}

ConstCx_Block::ConstCx_Block(const std::string& name)
	: Block(name)
	, m_value(0.0, 0.0)
	, m_showAdvancedParams(ConstCx::No)
	, m_sampleRateOption(ConstCx::TimedFromSchematic)
	, m_sampleRate(0.0)
	, m_initialDelay(0)
{
}

void ConstCx_Block::SetDefaultParamters()
{
	m_value = std::complex<double>(0.0, 0.0);
	m_showAdvancedParams = ConstCx::No;
	m_sampleRateOption = ConstCx::TimedFromSchematic;
	m_sampleRate = getSimu().samplingRate;
	m_initialDelay = 0;
}

std::complex<double> ConstCx_Block::ParseComplexValue(const std::string& value)
{
	try {
		auto mat = DataTypesAndParsers::ParseStringToMatrixDComplex(value);
		if (mat.NumElements() > 0) {
			return mat(0);
		}
	} catch (...) {
	}

	try {
		double real = std::stod(value);
		return std::complex<double>(real, 0.0);
	} catch (...) {
	}

	return std::complex<double>(0.0, 0.0);
}

void ConstCx_Block::SetParameters()
{
	if (!m_constCx) {
		return;
	}

	m_constCx->Value = m_value;
	m_constCx->ShowAdvancedParams = m_showAdvancedParams;
	m_constCx->SampleRateOption = m_sampleRateOption;
	m_constCx->SampleRate = m_sampleRate;
	m_constCx->InitialDelay = m_initialDelay;
}

bool ConstCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool ConstCx_Block::Run()
{

    if (!m_constCx) {
		return false;
	}

	if (!m_constCx->Run()) {
		return false;
	}

	std::vector<std::complex<double>> outputData;
	outputData.push_back(m_constCx->output[0U]);
	WriteOutputData(GetOutputPortName(0), outputData);

	m_constCx->Advance();

	return true;
}

bool ConstCx_Block::Initialize()
{
	SetBlockType(Block::BlockType::SOURCE);

	m_constCx = std::make_unique<ConstCx>();

	AddOutputPort("output", m_constCx->output, 1, Block::DataType::TIMED_DCOMPLEX);

	SetDefaultParamters();

	try { m_value = ParseComplexValue(getParameter("Value").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Value', using default value."); }
	try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
	try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
	try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
	
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }

	SetParameters();

	if (!m_constCx->Setup()) {
		return false;
	}

	return true;
}

ConstCx::SelectedShowAdvancedParams ConstCx_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return ConstCx::No;
    }
	if (lower == "yes" || lower == "1") {
		return ConstCx::Yes;
	}
	return ConstCx::No;
}

ConstCx::SelectedSampleRateOption ConstCx_Block::ConvertStringToSampleRateOption(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "untimed" || lower == "0") {
		return ConstCx::UnTimed;
	}
	if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
		return ConstCx::TimedFromSampleRate;
	}
	if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
		return ConstCx::TimedFromSchematic;
	}
	return ConstCx::TimedFromSchematic;
}







