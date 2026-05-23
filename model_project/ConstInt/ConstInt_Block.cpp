#include "ConstInt_Block.h"
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

ConstInt_Block::ConstInt_Block(const std::string& name)
	: Block(name)
	, m_value(0)
	, m_showAdvancedParams(ConstInt::No)
	, m_sampleRateOption(ConstInt::TimedFromSchematic)
	, m_sampleRate(0.0)
	, m_initialDelay(0)
{
}

void ConstInt_Block::SetDefaultParamters()
{
	m_value = 0;
	m_showAdvancedParams = ConstInt::No;
	m_sampleRateOption = ConstInt::TimedFromSchematic;
	m_sampleRate = getSimu().samplingRate;
	m_initialDelay = 0;
}

void ConstInt_Block::SetParameters()
{
	if (!m_constInt) {
		return;
	}

	m_constInt->Value = m_value;
	m_constInt->ShowAdvancedParams = m_showAdvancedParams;
	m_constInt->SampleRateOption = m_sampleRateOption;
	m_constInt->SampleRate = m_sampleRate;
	m_constInt->InitialDelay = m_initialDelay;
}

bool ConstInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool ConstInt_Block::Run()
{
    if (!m_constInt) {
        return false;
    }

	if (!m_constInt->Run()) {
		return false;
	}

	std::vector<int> outputData;
	outputData.push_back(m_constInt->output[0U]);
	WriteOutputData(GetOutputPortName(0), outputData);

	m_constInt->Advance();

	return true;
}

bool ConstInt_Block::Initialize()
{
	SetBlockType(Block::BlockType::SOURCE);

	m_constInt = std::make_unique<ConstInt>();

	AddOutputPort("output", m_constInt->output, 1, Block::DataType::TIMED_INT);

	SetDefaultParamters();

	try { m_value = std::stoi(getParameter("Value").Value); } catch (...) { }
	try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
	try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
	try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
	
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { }

	SetParameters();

	if (!m_constInt->Setup()) {
		return false;
	}

	return true;
}

ConstInt::SelectedShowAdvancedParams ConstInt_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return ConstInt::No;
    }
	if (lower == "yes" || lower == "1") {
		return ConstInt::Yes;
	}
	return ConstInt::No;
}

ConstInt::SelectedSampleRateOption ConstInt_Block::ConvertStringToSampleRateOption(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "untimed" || lower == "0") {
		return ConstInt::UnTimed;
	}
	if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
		return ConstInt::TimedFromSampleRate;
	}
	if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
		return ConstInt::TimedFromSchematic;
	}
	return ConstInt::TimedFromSchematic;
}






