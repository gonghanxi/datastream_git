#include "Window_Block.h"
#include <algorithm>
#include <cctype>
#include <vector>

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

Window_Block::Window_Block(const std::string& name)
    : Block(name)
{
}

void Window_Block::SetDefaultParamters()
{
    m_windowType = Window::Hanning;
    m_length = 256;
    m_zeroPad = 0;
    m_kaiserParameter = 1.0;
    m_showAdvancedParams = Window::No;
    m_sampleRateOption = Window::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
}

void Window_Block::SetParameters()
{
    if (!m_window) {
        return;
    }

    m_window->WindowType = m_windowType;
    m_window->Length = m_length;
    m_window->ZeroPad = m_zeroPad;
    m_window->KaiserParameter = m_kaiserParameter;
    m_window->ShowAdvancedParams = m_showAdvancedParams;
    m_window->SampleRateOption = m_sampleRateOption;
    m_window->SampleRate = m_sampleRate;
    m_window->InitialDelay = m_initialDelay;
}

bool Window_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Window_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }
    if (!m_window) {
        return false;
    }

    if (!m_window->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_window->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    m_window->Advance();

    return true;
}

bool Window_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_window = std::make_unique<Window>();

    AddOutputPort("output", m_window->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_windowType = ConvertStringToWindowType(getParameter("WindowType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'WindowType', using default value."); }
    try { m_length = std::stoi(getParameter("Length").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Length', using default value."); }
    try { m_zeroPad = std::stoi(getParameter("ZeroPad").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ZeroPad', using default value."); }
    try { m_kaiserParameter = std::stod(getParameter("KaiserParameter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'KaiserParameter', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    
    if (m_sampleRate <= 0.0) {
        LOG_ERROR("SampleRate must be greater than 0.");
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }

    SetParameters();

    if (!m_window->Setup()) {
        return false;
    }

    return true;
}

Window::SelectedWindowType Window_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle" || lower == "0") {
        return Window::Rectangle;
    }
    if (lower == "bartlett" || lower == "1") {
        return Window::Bartlett;
    }
    if (lower == "hanning" || lower == "2") {
        return Window::Hanning;
    }
    if (lower == "hamming" || lower == "3") {
        return Window::Hamming;
    }
    if (lower == "blackman" || lower == "4") {
        return Window::Blackman;
    }
    if (lower == "steepblackman" || lower == "5") {
        return Window::SteepBlackman;
    }
    if (lower == "kaiser" || lower == "6") {
        return Window::Kaiser;
    }
    return Window::Hanning;
}

Window::SelectedShowAdvancedParams Window_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Window::No;
    }
    if (lower == "yes" || lower == "1") {
        return Window::Yes;
    }
    return Window::No;
}

Window::SelectedSampleRateOption Window_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return Window::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return Window::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return Window::TimedFromSchematic;
    }
    return Window::TimedFromSchematic;
}
















