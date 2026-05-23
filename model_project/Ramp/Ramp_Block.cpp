#include "Ramp_Block.h"
#include <algorithm>
#include <cctype>
#include <vector>
#include <iostream>

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

Ramp_Block::Ramp_Block(const std::string& name)
    : Block(name)
{
}

void Ramp_Block::SetDefaultParamters()
{
    m_stepPerSample = 1.0;
    m_initialValue = 0.0;
    m_showAdvancedParams = Ramp::No;
    m_sampleRateOption = Ramp::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
}

void Ramp_Block::SetParameters()
{
    if (!m_ramp) {
        return;
    }

    m_ramp->StepPerSample = m_stepPerSample;
    m_ramp->InitialValue = m_initialValue;
    m_ramp->ShowAdvancedParams = m_showAdvancedParams;
    m_ramp->SampleRateOption = m_sampleRateOption;
    m_ramp->SampleRate = m_sampleRate;
    m_ramp->InitialDelay = m_initialDelay;
}

bool Ramp_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Ramp_Block::Run()
{
    if (!m_ramp) {
        return false;
    }

    if (!m_ramp->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_ramp->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    if (m_ramp) {
        m_ramp->Advance();
    }
    m_producedCount++;

    return true;
}

bool Ramp_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_ramp = std::make_unique<Ramp>();

    AddOutputPort("output", m_ramp->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_stepPerSample = std::stod(getParameter("StepPerSample").Value); } catch (...) { }
    try { m_initialValue = std::stod(getParameter("InitialValue").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { }

    SetParameters();

    if (!m_ramp->Setup()) {
        return false;
    }

    return true;
}


int Ramp_Block::GetBatchSize() const
{
//    // 获取下游Buffer使用率
//    float u = GetDownstreamBufferUsage();

//    if(u > 80.0f) return 1;
//    else if (u > 60.0f) return std::max(1, m_batchSize / 4);
//    else if (u > 40.0f) return std::max(1, m_batchSize / 2);
//    else if (u < 20.0f && m_batchSize < 50) return std::min(50, m_batchSize * 2);
    return m_batchSize;
}

int Ramp_Block::RunBatch(int maxCount)
{
    if (!m_ramp) return 0;

    // 计算本次可生产的数量
    size_t totalSamples = getSimu().num_Samples;
    size_t remaining = (m_producedCount >= totalSamples) ? 0 : (totalSamples - m_producedCount);
    int batchSize = std::min(maxCount, (int)remaining);
    batchSize = std::min(batchSize, GetBatchSize());

    if (batchSize <= 0) {
        if (m_producedCount >= totalSamples) {
            SetDone(true);
            Stop();
        }
        return 0;
    }

    Buffer* outBuffer = GetOutputPort(GetOutputPortName(0));
    if (outBuffer && outBuffer->GetReaderCount() > 0) {
        size_t freeSpace = outBuffer->GetBufferFreeSpace();
        if (freeSpace < static_cast<size_t>(batchSize)) {
            batchSize = static_cast<int>(freeSpace);
            if (batchSize <= 0) return 0;
        }
    }

    std::vector<double> outputData;
    outputData.reserve(batchSize);

    for (int i = 0; i < batchSize; i++) {
        if (!m_ramp->Run()) {
            break;
        }
        outputData.push_back(m_ramp->output[0U]);
        m_ramp->Advance();
    }

    if (outputData.empty()) return 0;

    if (!WriteOutputData(GetOutputPortName(0), outputData)) {
        return 0;
    }

    m_producedCount += outputData.size();

    if (m_producedCount >= totalSamples) {
        SetDone(true);
        Stop();
    }
//    qDebug() << "Ramp_Block --" << QString::fromStdString(GetName()) << " 单次成功产生" << outputData.size() << "个 数据";

    return (int)outputData.size();
}

Ramp::SelectedShowAdvancedParams Ramp_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Ramp::No;
    }
    if (lower == "yes" || lower == "1") {
        return Ramp::Yes;
    }
    return Ramp::No;
}

Ramp::SelectedSampleRateOption Ramp_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return Ramp::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return Ramp::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return Ramp::TimedFromSchematic;
    }
    return Ramp::TimedFromSchematic;
}
















