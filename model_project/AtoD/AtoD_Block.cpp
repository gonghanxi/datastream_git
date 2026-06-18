#include "AtoD_Block.h"
#include <algorithm>
#include <cmath>

namespace {
const double kPi = 3.1415926535897932384626433832795;
const double kTiny = 1e-30;

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

double clip(double x, double lo, double hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

int clampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
}

AtoD_Block::AtoD_Block(const std::string& name)
    : Block(name)
    , m_sampleIndex(0)
    , m_sdIIntegrator(0.0)
    , m_sdQIntegrator(0.0)
    , m_sdIFeedback(0.0)
    , m_sdQFeedback(0.0)
    , m_sdIAccum(0.0)
    , m_sdQAccum(0.0)
    , m_sdAccumCount(0)
    , m_sdHeldOutput(0.0, 0.0)
    , m_rng(0x12345678)
    , m_hasClockState(false)
    , m_lastClockValue(0.0)
    , m_heldSample(0.0, 0.0)
    , m_hasPendingClockSample(false)
    , m_pendingClockSample(0.0, 0.0)
    , m_hasRawInputState(false)
    , m_prevRawInputTime(0.0)
    , m_prevRawInput(0.0, 0.0)
    , m_hasNextClockCrossing(false)
    , m_nextClockCrossingTime(0.0)
{
}

bool AtoD_Block::parseArrayString(const std::string &arrayStr, std::vector<double> &outArray)
{
    outArray.clear();

    std::string str = arrayStr;
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    std::string content = str.substr(1, str.length() - 2);

    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                int value = std::stoi(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}

void AtoD_Block::SetDefaultParamters()
{
    m_NBits = 8;
    m_VRef = 1.0;
    m_ADCType = AtoD::Current_AtoD;
    m_OutputDigitalFormat = AtoD::Offset_binary;
    m_DistortionModel = AtoD::Jitter_INL_DNL;
    m_EnableJitter = AtoD::Jitter_No;
    m_RJrms = 0.0;
    m_PhaseNoiseData = nullptr;
    m_PhaseNoiseDataSize = 0;
    m_PN_Type = AtoD::Random_PN;

    m_INL = 0.0;
    m_DNL = 0.0;

    m_ENOB = 7;
    m_SNR_dB = 60.0;
    m_H2_dBc = -400.0;
    m_H3_dBc = -400.0;
    m_H4_dBc = -400.0;
    m_H5_dBc = -400.0;

    m_SINAD_dB = 60.0;
    m_SFDR_dBc = 70.0;
    m_FFT_Size = AtoD::FFT_2_14;

    m_SNR_Model = AtoD::Quantization_and_Jitter;
    m_ThermalNoise_SNR_dBFS = 63;
    m_CenterFreq = 100.0e6;
    m_Level_dBFS = 0.0;

    m_ConversionType = AtoD::Clocked;
    m_Clock = 0.2e6;
    m_Phase = 0.0;

    m_DownsampleFactor = 1;
    m_DownsamplePhase = 0;
    m_AntiAliasingFilter = AtoD::AA_OFF;
    m_ExcessBW = 0.5;

    m_PipelineStageBits = 1;
    m_PipelineLatency = 0;
    m_SigmaDeltaOrder = 1;
    m_SigmaDeltaOSR = 16;
}

void AtoD_Block::SetParameters()
{
    m_PhaseNoiseData = primdata.data();
    m_PhaseNoiseDataSize = static_cast<double>(primdata.size());
}

void AtoD_Block::initQuantizationParams()
{
    m_nbits = clampInt(m_NBits, 4, 16);
    
    if (m_VRef <= 0.0)
        m_vref = 1.0;
    else
        m_vref = m_VRef;
    
    m_codeCount = 1 << m_nbits;
    m_midCode = m_codeCount / 2;
    m_lsb = 2.0 * m_vref / static_cast<double>(m_codeCount);
    
    m_sampleIndex = 0;
    m_pipelineFifo.clear();
    
    m_sdIIntegrator = 0.0;
    m_sdQIntegrator = 0.0;
    m_sdIFeedback = 0.0;
    m_sdQFeedback = 0.0;
    m_sdIAccum = 0.0;
    m_sdQAccum = 0.0;
    m_sdAccumCount = 0;
    m_sdHeldOutput = std::complex<double>(0.0, 0.0);
}

AtoD_Block::QuantResult AtoD_Block::quantize(double x) const
{
    QuantResult r;
    double xc = clip(x, -m_vref, m_vref);
    
    double u = (xc + m_vref) / m_lsb;
    int code = static_cast<int>(std::floor(u));
    code = clampInt(code, 0, m_codeCount - 1);
    
    r.codeOffset = code;
    r.analog = -m_vref + (static_cast<double>(code) + 0.5) * m_lsb;
    
    if (m_OutputDigitalFormat == AtoD::Twos_complement)
        r.codeDigital = code - m_midCode;
    else
        r.codeDigital = code;
    
    return r;
}

AtoD_Block::QuantResult AtoD_Block::quantizeFlash(double x) const
{
    QuantResult r;
    double xc = clip(x, -m_vref, m_vref);
    int code = 0;
    
    for (int k = 1; k < m_codeCount; ++k) {
        double th = -m_vref + static_cast<double>(k) * m_lsb;
        if (xc >= th)
            ++code;
        else
            break;
    }
    
    code = clampInt(code, 0, m_codeCount - 1);
    r.codeOffset = code;
    r.analog = -m_vref + (static_cast<double>(code) + 0.5) * m_lsb;
    
    if (m_OutputDigitalFormat == AtoD::Twos_complement)
        r.codeDigital = code - m_midCode;
    else
        r.codeDigital = code;
    
    return r;
}

std::complex<double> AtoD_Block::processPipeline(const std::complex<double>& x)
{
    if (m_PipelineLatency <= 0)
        return x;
    
    m_pipelineFifo.push_back(x);
    
    if (static_cast<int>(m_pipelineFifo.size()) <= m_PipelineLatency)
        return std::complex<double>(0.0, 0.0);
    
    std::complex<double> y = m_pipelineFifo.front();
    m_pipelineFifo.erase(m_pipelineFifo.begin());
    return y;
}

std::complex<double> AtoD_Block::processSigmaDelta(const std::complex<double>& x)
{
    double ui = clip(x.real() / std::max(m_vref, kTiny), -1.0, 1.0);
    double uq = clip(x.imag() / std::max(m_vref, kTiny), -1.0, 1.0);
    
    m_sdIIntegrator += ui - m_sdIFeedback;
    double bi = (m_sdIIntegrator >= 0.0) ? 1.0 : -1.0;
    m_sdIFeedback = bi;
    
    m_sdQIntegrator += uq - m_sdQFeedback;
    double bq = (m_sdQIntegrator >= 0.0) ? 1.0 : -1.0;
    m_sdQFeedback = bq;
    
    m_sdIAccum += bi;
    m_sdQAccum += bq;
    ++m_sdAccumCount;
    
    int osr = std::max(1, m_SigmaDeltaOSR);
    if (m_sdAccumCount >= osr) {
        double ai = m_sdIAccum / static_cast<double>(m_sdAccumCount);
        double aq = m_sdQAccum / static_cast<double>(m_sdAccumCount);
        
        m_sdHeldOutput = std::complex<double>(
            clip(ai * m_vref, -m_vref, m_vref),
            clip(aq * m_vref, -m_vref, m_vref));
        
        m_sdIAccum = 0.0;
        m_sdQAccum = 0.0;
        m_sdAccumCount = 0;
    }
    else if (m_sampleIndex == 0) {
        m_sdHeldOutput = std::complex<double>(bi * m_vref, bq * m_vref);
    }
    
    return m_sdHeldOutput;
}

double AtoD_Block::firstPositiveCrossingAtOrAfter(double t) const
{
    const double period = 1.0 / std::max(m_Clock, kTiny);
    const double phaseRad = m_Phase * kPi / 180.0;
    const double base = ((1.5 * kPi) - phaseRad) / (2.0 * kPi * std::max(m_Clock, kTiny));

    double k = std::ceil((t - base) / period - 1e-12);
    if (k < 0.0)
        k = 0.0;

    double ts = base + k * period;
    while (ts < t - 1e-15)
        ts += period;

    return ts;
}

std::complex<double> AtoD_Block::interpSample(const std::complex<double>& x0, double t0,
                                               const std::complex<double>& x1, double t1, double ts) const
{
    if (std::fabs(t1 - t0) <= kTiny)
        return x1;

    double a = (ts - t0) / (t1 - t0);
    a = clip(a, 0.0, 1.0);
    return x0 + (x1 - x0) * a;
}

std::complex<double> AtoD_Block::getClockInput(const std::complex<double>& x, double t)
{
    if (m_Clock <= 0.0)
        return x;

    const double phaseRad = m_Phase * kPi / 180.0;
    const double c = std::cos(2.0 * kPi * m_Clock * t + phaseRad);
    const double period = 1.0 / std::max(m_Clock, kTiny);

    if (!m_hasClockState)
    {
        m_heldSample = std::complex<double>(0.0, 0.0);
        m_hasPendingClockSample = false;
        m_pendingClockSample = std::complex<double>(0.0, 0.0);

        m_lastClockValue = c;
        m_hasClockState = true;

        m_hasRawInputState = true;
        m_prevRawInputTime = t;
        m_prevRawInput = x;

        m_nextClockCrossingTime = firstPositiveCrossingAtOrAfter(t);
        m_hasNextClockCrossing = true;

        if (m_nextClockCrossingTime <= t + 1e-15)
        {
            m_pendingClockSample = x;
            m_hasPendingClockSample = true;
            m_nextClockCrossingTime += period;
        }

        return m_heldSample;
    }

    if (m_hasPendingClockSample)
    {
        m_heldSample = m_pendingClockSample;
        m_hasPendingClockSample = false;
    }

    const std::complex<double> y = m_heldSample;

    if (!m_hasRawInputState)
    {
        m_hasRawInputState = true;
        m_prevRawInputTime = t;
        m_prevRawInput = x;
        m_lastClockValue = c;
        return y;
    }

    if (t < m_prevRawInputTime - 1e-15)
    {
        m_prevRawInputTime = t;
        m_prevRawInput = x;
        m_nextClockCrossingTime = firstPositiveCrossingAtOrAfter(t);
        m_hasNextClockCrossing = true;
        m_lastClockValue = c;
        return y;
    }

    if (!m_hasNextClockCrossing)
    {
        m_nextClockCrossingTime = firstPositiveCrossingAtOrAfter(m_prevRawInputTime);
        m_hasNextClockCrossing = true;
    }

    while (m_nextClockCrossingTime <= t + 1e-15)
    {
        if (m_nextClockCrossingTime >= m_prevRawInputTime - 1e-15)
        {
            m_pendingClockSample = interpSample(m_prevRawInput, m_prevRawInputTime, x, t, m_nextClockCrossingTime);
            m_hasPendingClockSample = true;
        }
        m_nextClockCrossingTime += period;
    }

    m_prevRawInputTime = t;
    m_prevRawInput = x;
    m_lastClockValue = c;

    return y;
}

bool AtoD_Block::Setup()
{
    Block::Setup();
    m_inputBuffer.clear();
    while (!m_aoutQueue.empty()) m_aoutQueue.pop();
    while (!m_diQueue.empty()) m_diQueue.pop();
    while (!m_dqQueue.empty()) m_dqQueue.pop();
    
    // Reset algorithm state
    m_sampleIndex = 0;
    m_pipelineFifo.clear();
    m_sdIIntegrator = 0.0;
    m_sdQIntegrator = 0.0;
    m_sdIFeedback = 0.0;
    m_sdQFeedback = 0.0;
    m_sdIAccum = 0.0;
    m_sdQAccum = 0.0;
    m_sdAccumCount = 0;
    m_sdHeldOutput = std::complex<double>(0.0, 0.0);

    // Reset clocked input state
    m_hasClockState = false;
    m_lastClockValue = 0.0;
    m_heldSample = std::complex<double>(0.0, 0.0);
    m_hasPendingClockSample = false;
    m_pendingClockSample = std::complex<double>(0.0, 0.0);
    m_hasRawInputState = false;
    m_prevRawInputTime = 0.0;
    m_prevRawInput = std::complex<double>(0.0, 0.0);
    m_hasNextClockCrossing = false;
    m_nextClockCrossingTime = 0.0;
    
    return true;
}

bool AtoD_Block::Run()
{
    if (IsVariableStepMode() || m_inputRate > 1) return TimeDrivenRun();
    return DataStreamRun();
}

bool AtoD_Block::DataStreamRun()
{
    std::string A_INPortName = GetInputPortName(0);
    std::string A_outPortName = GetOutputPortName(0);
    std::string D_IPortName = GetOutputPortName(1);
    std::string D_QPortName = GetOutputPortName(2);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(A_INPortName);
    if (inputData.empty()) {
        if (IsVariableStepMode()) {
            return true;
        }
        return false;
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputA_OUTData;
    std::vector<double> outputD_IData;
    std::vector<double> outputD_QData;

    for (size_t idx = 0; idx < inputData.size(); ++idx) {
        std::complex<double> rawX = inputData[idx].complex();

        // Apply clocked input (sample-and-hold) for Clocked mode
        std::complex<double> x;
        if (m_ConversionType == AtoD::Clocked) {
            double t = simulator_param.startTime + static_cast<double>(m_sampleIndex) * simulator_param.time_Interval;
            x = getClockInput(rawX, t);
        } else {
            x = rawX;
        }
        
        QuantResult qi, qq;
        
        if (m_ADCType == AtoD::Flash_ADC) {
            qi = quantizeFlash(x.real());
            qq = quantizeFlash(x.imag());
        }
        else if (m_ADCType == AtoD::Pipeline_ADC) {
            std::complex<double> xp = processPipeline(x);
            qi = quantize(xp.real());
            qq = quantize(xp.imag());
        }
        else if (m_ADCType == AtoD::SigmaDelta_ADC) {
            std::complex<double> xs = processSigmaDelta(x);
            qi = quantize(xs.real());
            qq = quantize(xs.imag());
        }
        else {
            // Current AtoD - simple quantization
            qi = quantize(x.real());
            qq = quantize(x.imag());
        }
        
        outputA_OUTData.push_back(SystemVueModelBuilder::EnvelopeSignal(std::complex<double>(qi.analog, qq.analog)));
        outputD_IData.push_back(static_cast<double>(qi.codeDigital));
        outputD_QData.push_back(static_cast<double>(qq.codeDigital));
        
        ++m_sampleIndex;
    }

    WriteOutputData(A_outPortName, outputA_OUTData);
    WriteOutputData(D_IPortName, outputD_IData);
    WriteOutputData(D_QPortName, outputD_QData);

    m_atod->Advance();

    return true;
}

bool AtoD_Block::TimeDrivenRun()
{
    std::string A_INPortName = GetInputPortName(0);
    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(A_INPortName);
    if (inputData.empty()) {
        if (IsVariableStepMode()) {
            return true;
        }
        return false;
    }
    
    for (size_t i = 0; i < inputData.size(); ++i) {
        m_inputBuffer.push_back(inputData[i]);
    }

    if (static_cast<int>(m_inputBuffer.size()) >= m_inputRate) {
        // Get the sample at the downsample phase index
        int phase = clampInt(m_DownsamplePhase, 0, m_inputRate - 1);
        std::complex<double> rawX = m_inputBuffer[phase].complex();

        // Apply clocked input (sample-and-hold) for Clocked mode
        std::complex<double> x;
        if (m_ConversionType == AtoD::Clocked) {
            double t = simulator_param.startTime + static_cast<double>(m_sampleIndex) * simulator_param.time_Interval;
            x = getClockInput(rawX, t);
        } else {
            x = rawX;
        }
        
        QuantResult qi, qq;
        
        if (m_ADCType == AtoD::Flash_ADC) {
            qi = quantizeFlash(x.real());
            qq = quantizeFlash(x.imag());
        }
        else if (m_ADCType == AtoD::Pipeline_ADC) {
            std::complex<double> xp = processPipeline(x);
            qi = quantize(xp.real());
            qq = quantize(xp.imag());
        }
        else if (m_ADCType == AtoD::SigmaDelta_ADC) {
            std::complex<double> xs = processSigmaDelta(x);
            qi = quantize(xs.real());
            qq = quantize(xs.imag());
        }
        else {
            qi = quantize(x.real());
            qq = quantize(x.imag());
        }
        
        m_aoutQueue.push(SystemVueModelBuilder::EnvelopeSignal(std::complex<double>(qi.analog, qq.analog)));
        m_diQueue.push(static_cast<double>(qi.codeDigital));
        m_dqQueue.push(static_cast<double>(qq.codeDigital));
        
        ++m_sampleIndex;
        m_inputBuffer.clear();
    }

    std::string A_outPortName = GetOutputPortName(0);
    std::string D_IPortName = GetOutputPortName(1);
    std::string D_QPortName = GetOutputPortName(2);

    if (!m_aoutQueue.empty()) {
        std::vector<SystemVueModelBuilder::EnvelopeSignal> outA{ m_aoutQueue.front() };
        m_aoutQueue.pop();
        WriteOutputData(A_outPortName, outA);
    }
    if (!m_diQueue.empty()) {
        std::vector<double> outI{ m_diQueue.front() };
        m_diQueue.pop();
        WriteOutputData(D_IPortName, outI);
    }
    if (!m_dqQueue.empty()) {
        std::vector<double> outQ{ m_dqQueue.front() };
        m_dqQueue.pop();
        WriteOutputData(D_QPortName, outQ);
    }

    m_atod->Advance();

    return true;
}

bool AtoD_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_NBits = std::stod(getParameter("NBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NBits', using default value."); }
    try { m_VRef = std::stod(getParameter("VRef").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'VRef', using default value."); }
    try { m_OutputDigitalFormat = ConvertStringToOutputDigitalFormatEnum(getParameter("OutputDigitalFormat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputDigitalFormat', using default value."); }
    try { m_DistortionModel = ConvertStringToDistortionModelEnum(getParameter("DistortionModel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DistortionModel', using default value."); }
    try { m_EnableJitter = ConvertStringToEnableJitterEnum(getParameter("EnableJitter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'EnableJitter', using default value."); }
    try { m_RJrms = std::stod(getParameter("RJrms").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RJrms', using default value."); }
    try {
        std::string PrimString = getParameter("PhaseNoiseData").Value;
        parseArrayString(PrimString, primdata);
    } catch(...) { LOG_WARN("Failed to parse parameter 'PhaseNoiseData', using default value."); }
    try { m_PN_Type = ConvertStringToPN_TypeEnum(getParameter("PN_Type").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PN_Type', using default value."); }

    try { m_INL = std::stod(getParameter("INL").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'INL', using default value."); }
    try { m_DNL = std::stod(getParameter("DNL").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DNL', using default value."); }

    try { m_ENOB = std::stoi(getParameter("ENOB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ENOB', using default value."); }
    try { m_SNR_dB = std::stod(getParameter("SNR_dB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SNR_dB', using default value."); }
    try { m_H2_dBc = std::stod(getParameter("H2_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H2_dBc', using default value."); }
    try { m_H3_dBc = std::stod(getParameter("H3_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H3_dBc', using default value."); }
    try { m_H4_dBc = std::stod(getParameter("H4_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H4_dBc', using default value."); }
    try { m_H5_dBc = std::stod(getParameter("H5_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H5_dBc', using default value."); }

    try { m_SINAD_dB = std::stod(getParameter("SINAD_dB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SINAD_dB', using default value."); }
    try { m_SFDR_dBc = std::stod(getParameter("SFDR_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SFDR_dBc', using default value."); }
    try { m_FFT_Size = ConvertStringToFFT_SizeEnum(getParameter("FFT_Size").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FFT_Size', using default value."); }

    try { m_SNR_Model = ConvertStringToSNR_ModelEnum(getParameter("SNR_Model").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SNR_Model', using default value."); }
    try { m_ThermalNoise_SNR_dBFS = std::stoi(getParameter("ThermalNoise_SNR_dBFS").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ThermalNoise_SNR_dBFS', using default value."); }
    try { m_CenterFreq = std::stod(getParameter("CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CenterFreq', using default value."); }
    try { m_Level_dBFS = std::stod(getParameter("Level_dBFS").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Level_dBFS', using default value."); }

    try { m_ConversionType = ConvertStringToConversionTypeEnum(getParameter("ConversionType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ConversionType', using default value."); }
    try { m_Clock = std::stod(getParameter("Clock").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Clock', using default value."); }
    try { m_Phase = std::stod(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }

    try { m_DownsampleFactor = std::stoi(getParameter("DownsampleFactor").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DownsampleFactor', using default value."); }
    try { m_DownsamplePhase = std::stoi(getParameter("DownsamplePhase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DownsamplePhase', using default value."); }
    try { m_AntiAliasingFilter = ConvertStringToAntiAliasingFilterEnum(getParameter("AntiAliasingFilter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntiAliasingFilter', using default value."); }
    try { m_ExcessBW = std::stod(getParameter("ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ExcessBW', using default value."); }

    try { m_ADCType = ConvertStringToADCTypeEnum(getParameter("ADCType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ADCType', using default value."); }
    try { m_PipelineStageBits = std::stoi(getParameter("PipelineStageBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PipelineStageBits', using default value."); }
    try { m_PipelineLatency = std::stoi(getParameter("PipelineLatency").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PipelineLatency', using default value."); }
    try { m_SigmaDeltaOrder = std::stoi(getParameter("SigmaDeltaOrder").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SigmaDeltaOrder', using default value."); }
    try { m_SigmaDeltaOSR = std::stoi(getParameter("SigmaDeltaOSR").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SigmaDeltaOSR', using default value."); }

    SetParameters();
    initQuantizationParams();

    // Determine input rate
    if (m_ConversionType == AtoD::Downsampled) {
        m_inputRate = m_DownsampleFactor;
    } else {
        m_inputRate = 1;
    }

    // Create m_atod only for port buffers
    m_atod = std::make_unique<AtoD>();
    m_atod->A_in.SetStartTime(simulator_param.startTime);
    m_atod->A_out.SetStartTime(simulator_param.startTime);

    // Register ports
    AddInputPort("A_in", m_atod->A_in, m_inputRate, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("A_out", m_atod->A_out, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("D_I", m_atod->D_I, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("D_Q", m_atod->D_Q, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

AtoD::OutputDigitalFormatEnum AtoD_Block::ConvertStringToOutputDigitalFormatEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "offset_binary") {
        return AtoD::Offset_binary;
    }
    if (lower == "twos_complement" || lower == "1") {
        return AtoD::Twos_complement;
    }
    return AtoD::Offset_binary;
}

AtoD::DistortionModelEnum AtoD_Block::ConvertStringToDistortionModelEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "distortion_none") {
        return AtoD::Distortion_None;
    }
    if (lower == "jitter_inl_dnl" || lower == "1") {
        return AtoD::Jitter_INL_DNL;
    }
    if (lower == "enob_value" || lower == "2") {
        return AtoD::ENOB_value;
    }
    if (lower == "snr_and_harmonics" || lower == "3") {
        return AtoD::SNR_and_Harmonics;
    }
    if (lower == "sinad_and_sfdr" || lower == "4") {
        return AtoD::SINAD_and_SFDR;
    }
    return AtoD::Distortion_None;
}

AtoD::EnableJitterEnum AtoD_Block::ConvertStringToEnableJitterEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "jitter_no") {
        return AtoD::Jitter_No;
    }
    if (lower == "time_domain" || lower == "1") {
        return AtoD::Time_Domain;
    }
    if (lower == "frequency_domain" || lower == "2") {
        return AtoD::Frequency_Domain;
    }
    return AtoD::Jitter_No;
}

AtoD::PN_TypeEnum AtoD_Block::ConvertStringToPN_TypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "random_pn") {
        return AtoD::Random_PN;
    }
    if (lower == "fixed_freq_offset" || lower == "1") {
        return AtoD::Fixed_freq_offset;
    }
    if (lower == "fixed_freq_offset_and_amplitude" || lower == "2") {
        return AtoD::Fixed_freq_offset_and_amplitude;
    }
    return AtoD::Random_PN;
}

AtoD::FFT_SizeEnum AtoD_Block::ConvertStringToFFT_SizeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "fft_2_12") {
        return AtoD::FFT_2_12;
    }
    if (lower == "fft_2_13" || lower == "1") {
        return AtoD::FFT_2_13;
    }
    if (lower == "fft_2_14" || lower == "2") {
        return AtoD::FFT_2_14;
    }
    if (lower == "fft_2_15" || lower == "3") {
        return AtoD::FFT_2_15;
    }
    if (lower == "fft_2_16" || lower == "4") {
        return AtoD::FFT_2_16;
    }
    return AtoD::FFT_2_12;
}

AtoD::SNR_ModelEnum AtoD_Block::ConvertStringToSNR_ModelEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "quantization_and_jitter") {
        return AtoD::Quantization_and_Jitter;
    }
    if (lower == "quantization_and_inl_dnl" || lower == "1") {
        return AtoD::Quantization_and_INL_DNL;
    }
    if (lower == "quantization_and_jitter_or_inl_dnl" || lower == "2") {
        return AtoD::Quantization_and_Jitter_or_INL_DNL;
    }
    if (lower == "quantization_jitter_and_thermal_noise" || lower == "3") {
        return AtoD::Quantization_Jitter_and_Thermal_Noise;
    }
    return AtoD::Quantization_and_Jitter;
}

AtoD::ConversionTypeEnum AtoD_Block::ConvertStringToConversionTypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "clocked") {
        return AtoD::Clocked;
    }
    if (lower == "downsampled" || lower == "1") {
        return AtoD::Downsampled;
    }
    return AtoD::Clocked;
}

AtoD::AntiAliasingFilterEnum AtoD_Block::ConvertStringToAntiAliasingFilterEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "aa_off") {
        return AtoD::AA_OFF;
    }
    if (lower == "aa_on" || lower == "1") {
        return AtoD::AA_ON;
    }
    return AtoD::AA_OFF;
}

AtoD::ADCTypeEnum AtoD_Block::ConvertStringToADCTypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "current_atod" || lower == "current") {
        return AtoD::Current_AtoD;
    }
    if (lower == "flash" || lower == "flash_adc" || lower == "1") {
        return AtoD::Flash_ADC;
    }
    if (lower == "pipeline" || lower == "pipeline_adc" || lower == "2") {
        return AtoD::Pipeline_ADC;
    }
    if (lower == "sigmadelta" || lower == "sigma_delta" || lower == "sigmadelta_adc" || lower == "3") {
        return AtoD::SigmaDelta_ADC;
    }
    return AtoD::Current_AtoD;
}
