#include "RADAR_MatchedFilter_Block.h"
#include <complex>
#include <iostream>
#include <vector>

RADAR_MatchedFilter_Block::RADAR_MatchedFilter_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_MatchedFilter_Block::SetDefaultParamters()
{
    m_pulseWidth = 1e-5;
    m_pri = 1e-4;
    m_sampleRate = 10e6;
}

bool RADAR_MatchedFilter_Block::DataStreamRun()
{
    const std::string signalPort = GetInputPortName(0);
    const std::string referencePort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    const int numPRI = static_cast<int>(m_pri * m_sampleRate);
    const int numPulse = static_cast<int>(m_pulseWidth * m_sampleRate);

    // Accumulate all arrived chunks instead of keeping only one chunk.
    auto signalData = ReadInputData<std::complex<double>>(signalPort);
    if (!signalData.empty()) {
        m_cachedSignal.insert(m_cachedSignal.end(), signalData.begin(), signalData.end());
    }
    auto referenceData = ReadInputData<std::complex<double>>(referencePort);
    if (!referenceData.empty()) {
        m_cachedReference.insert(m_cachedReference.end(), referenceData.begin(), referenceData.end());
    }

    if (m_cachedSignal.size() < static_cast<size_t>(numPRI) ||
        m_cachedReference.size() < static_cast<size_t>(numPulse)) {
        return true;
    }

    std::vector<std::complex<double>> outputData;
    outputData.assign(static_cast<size_t>(numPRI), std::complex<double>(0.0, 0.0));

    SystemVueModelBuilder::Matrix<std::complex<double>> filterSequence(1, static_cast<size_t>(numPulse));
    for (int i = 0; i < numPulse; ++i) {
        filterSequence(static_cast<size_t>(numPulse - i - 1)) = std::conj(m_cachedReference[static_cast<size_t>(i)]);
    }

    for (int n = 0; n < numPRI; ++n) {
        for (int k = 0; k < numPulse; ++k) {
            int n_k = n - k;
            n_k = n - k < 0 ? n - k + numPRI : n - k;
            outputData[static_cast<size_t>(n_k)] += m_cachedSignal[static_cast<size_t>(n)] * filterSequence(static_cast<size_t>(k));
        }
    }

    WriteOutputData(outputPort, outputData);
    m_cachedSignal.erase(m_cachedSignal.begin(), m_cachedSignal.begin() + numPRI);
    m_cachedReference.erase(m_cachedReference.begin(), m_cachedReference.begin() + numPulse);

    return true;
}

bool RADAR_MatchedFilter_Block::TimeDrivenRun()
{
    const std::string signalPort = GetInputPortName(0);
    const std::string referencePort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    const size_t numPRI = static_cast<size_t>(m_pri * m_sampleRate);
    const size_t numPulse = static_cast<size_t>(m_pulseWidth * m_sampleRate);

    // Accumulate all arrived chunks instead of keeping only one chunk.
    auto signalData = ReadInputData<std::complex<double>>(signalPort);
    if (signalData.empty()) {
        return true;
    }
    auto referenceData = ReadInputData<std::complex<double>>(referencePort);
    if (referenceData.empty()) {
        return true;
    }

    for(const auto& val : signalData) m_signalBuffer.push_back(val);
    for(const auto& val : referenceData) m_referenceBuffer.push_back(val);

    if(m_signalBuffer.size() >= numPRI && m_referenceBuffer.size() >= numPulse) {
        m_cachedSignal.insert(m_cachedSignal.end(), m_signalBuffer.begin(), m_signalBuffer.end());
        m_cachedReference.insert(m_cachedReference.end(), m_referenceBuffer.begin(), m_referenceBuffer.end());

        std::vector<std::complex<double>> outputData;
        outputData.assign(static_cast<size_t>(numPRI), std::complex<double>(0.0, 0.0));

        SystemVueModelBuilder::Matrix<std::complex<double>> filterSequence(1, static_cast<size_t>(numPulse));
        for (int i = 0; i < static_cast<int>(numPulse); ++i) {
            filterSequence(static_cast<size_t>(numPulse - i - 1)) = std::conj(m_cachedReference[static_cast<size_t>(i)]);
        }

        for (int n = 0; n < static_cast<int>(numPRI); ++n) {
            for (int k = 0; k < static_cast<int>(numPulse); ++k) {
                int n_k = n - k;
                n_k = n - k < 0 ? n - k + numPRI : n - k;
                outputData[static_cast<size_t>(n_k)] += m_cachedSignal[static_cast<size_t>(n)] * filterSequence(static_cast<size_t>(k));
            }
        }

        for(const auto& val : signalData) m_outputQueue.push(val);
        if(!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_signalBuffer.clear();
            m_referenceBuffer.clear();
            qDebug() << "[RADAR_MatchedFilter_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << outputValue.imag();
        }
        m_cachedSignal.erase(m_cachedSignal.begin(), m_cachedSignal.begin() + numPRI);
        m_cachedReference.erase(m_cachedReference.begin(), m_cachedReference.begin() + numPulse);
    }
    return true;
}

void RADAR_MatchedFilter_Block::SetParameters(double pulseWidth, double pri, double sampleRate)
{
    m_pulseWidth = pulseWidth;
    m_pri = pri;
    m_sampleRate = sampleRate;

    if (m_radarMatchedFilter) {
        m_radarMatchedFilter->PulseWidth = m_pulseWidth;
        m_radarMatchedFilter->PRI = m_pri;
        m_radarMatchedFilter->SampleRate = m_sampleRate;
    }
}

bool RADAR_MatchedFilter_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RADAR_MatchedFilter_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_MatchedFilter_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarMatchedFilter = std::make_unique<RADAR_MatchedFilter>();

    SetDefaultParamters();

    try { m_pulseWidth = std::stod(getParameter("PulseWidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PulseWidth', using default value."); }
    try { m_pri = std::stod(getParameter("PRI").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    SetParameters(m_pulseWidth, m_pri, m_sampleRate);

    const size_t numPRI = static_cast<size_t>(m_pri * m_sampleRate);
    const size_t numPulse = static_cast<size_t>(m_pulseWidth * m_sampleRate);

    if (numPRI == 0 || numPulse == 0) {
        LOG_ERROR("error Port rate must be greater than 0. Check to make sure rate of reference: PulseWidth * SampleRate > 0 and rate of signal: PRI * SampleRate > 0.");
        return false;
    }

    AddInputPort("signal", m_radarMatchedFilter->signal, numPRI, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("reference", m_radarMatchedFilter->reference, numPulse, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_radarMatchedFilter->output, numPRI, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}
