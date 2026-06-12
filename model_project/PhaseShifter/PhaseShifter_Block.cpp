#include "PhaseShifter_Block.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

namespace {
std::string NormalizeEnumString(const std::string& value)
{
    std::string out;
    out.reserve(value.size());
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '_') {
            out.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return out;
}
}

PhaseShifter_Block::PhaseShifter_Block(const std::string& name)
    : Block(name)
    , m_rngN(12345)
    , m_rngU(54321)
{
}

void PhaseShifter_Block::SetDefaultParamters()
{
    m_phaseShift = 0.0;
    m_insertionLoss = 0.0;
    m_quantization = PhaseShifter::Quant_NO;
    m_numBits = 6;
    m_levels.Resize(1, 0);

    m_phaseShiftError = PhaseShifter::Err_None;
    m_customError = 0.0;
    m_stdDev = 3.0;
    m_min = -5.0;
    m_max = 5.0;

    m_sensitivity = 90.0;
    m_hilbertFilterLength = 64;

    m_L = 64;
    m_h.clear();
    m_x.clear();
}

void PhaseShifter_Block::SetParameters()
{
    if (!m_phaseShifter) {
        return;
    }

    m_phaseShifter->PhaseShift = m_phaseShift;
    m_phaseShifter->InsertionLoss = m_insertionLoss;
    m_phaseShifter->Quantization = m_quantization;
    m_phaseShifter->NumBits = m_numBits;
    m_phaseShifter->Levels = m_levels;

    m_phaseShifter->PhaseShiftError = m_phaseShiftError;
    m_phaseShifter->CustomError = m_customError;
    m_phaseShifter->StdDev = m_stdDev;
    m_phaseShifter->Min = m_min;
    m_phaseShifter->Max = m_max;

    m_phaseShifter->Sensitivity = m_sensitivity;
    m_phaseShifter->HilbertFilterLength = m_hilbertFilterLength;
}

bool PhaseShifter_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool PhaseShifter_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string controlPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    double fc = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();

    std::vector<double> controlData;
    const bool hasControl = GetInputPort(GetInputPortName(1))->IsConnected();
    if (hasControl) {
        controlData = ReadInputData<double>(controlPort);
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        double baseDeg = m_phaseShift;
        if (hasControl) {
            if (!controlData.empty()) {
                const size_t ci = (i < controlData.size()) ? i : (controlData.size() - 1);
                baseDeg = m_sensitivity * controlData[ci];
            } else {
                baseDeg = 0.0;
            }
        }

        const double phi = computePhaseRad(baseDeg);
        const double c = std::cos(phi);
        const double s = std::sin(phi);
        const double A = ampScale();

        SystemVueModelBuilder::EnvelopeSignal y;

        if (fc > 0.0) {
            auto cx = inputData[i].complex();
            auto rot = std::complex<double>(c, s);
            cx *= rot;
            cx *= std::complex<double>(A, 0.0);
            y = cx;
        } else {
            const double v = inputData[i].real();
            m_x.push_back(v);
            if ((int)m_x.size() > m_L) {
                m_x.pop_front();
            }

            const double v1h = hilbertConv();
            const double v1d = delayedReal();

            const double v2 = (v1d * c - v1h * s) * A;
            y = v2;
        }

        outputData.push_back(y);
    }
    WriteOutputData(outputPort, outputData);

    return true;
}

bool PhaseShifter_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string controlPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    double fc = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();

    std::vector<double> controlData;
    const bool hasControl = GetInputPort(GetInputPortName(1))->IsConnected();
    if (hasControl) {
        controlData = ReadInputData<double>(controlPort);
        if(controlData.empty()) return true;
        for(const auto& val : controlData) m_controlBuffer.push_back(val);
    }

    bool CanProcessData = false;
    if(hasControl) {
        if(m_inputBuffer.size() >= 1 && m_controlBuffer.size() >= 1) CanProcessData = true;
    }
    else {
        if(m_inputBuffer.size() >= 1) CanProcessData = true;
    }
    if(CanProcessData) {
        std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
        outputData.reserve(m_inputBuffer.size());

        for (size_t i = 0; i < m_inputBuffer.size(); ++i) {
            double baseDeg = m_phaseShift;
            if (hasControl) {
                if (!m_controlBuffer.empty()) {
                    const size_t ci = (i < m_controlBuffer.size()) ? i : (m_controlBuffer.size() - 1);
                    baseDeg = m_sensitivity * m_controlBuffer[ci];
                } else {
                    baseDeg = 0.0;
                }
            }

            const double phi = computePhaseRad(baseDeg);
            const double c = std::cos(phi);
            const double s = std::sin(phi);
            const double A = ampScale();

            SystemVueModelBuilder::EnvelopeSignal y;

            if (fc > 0.0) {
                auto cx = m_inputBuffer[i].complex();
                auto rot = std::complex<double>(c, s);
                cx *= rot;
                cx *= std::complex<double>(A, 0.0);
                y = cx;
            } else {
                const double v = m_inputBuffer[i].real();
                m_x.push_back(v);
                if ((int)m_x.size() > m_L) {
                    m_x.pop_front();
                }

                const double v1h = hilbertConv();
                const double v1d = delayedReal();

                const double v2 = (v1d * c - v1h * s) * A;
                y = v2;
            }

            outputData.push_back(y);
        }
        m_outputQueue.push(outputData[0]);
        //执行写入
        if (!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[PhaseShifter_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
            m_controlBuffer.clear();
        }
    }
    return true;
}

bool PhaseShifter_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool PhaseShifter_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_phaseShifter = std::make_unique<PhaseShifter>();

    AddInputPort("input", m_phaseShifter->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("control", m_phaseShifter->control, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_phaseShifter->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();

    try { m_phaseShift = std::stod(getParameter("PhaseShift").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseShift', using default value."); }
    try { m_insertionLoss = std::stod(getParameter("InsertionLoss").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InsertionLoss', using default value."); }
    try { m_quantization = ConvertStringToQuantEnum(getParameter("Quantization").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Quantization', using default value."); }
    try { m_numBits = std::stoi(getParameter("NumBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumBits', using default value."); }
    try { m_phaseShiftError = ConvertStringToErrEnum(getParameter("PhaseShiftError").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseShiftError', using default value."); }
    try { m_customError = std::stod(getParameter("CustomError").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CustomError', using default value."); }
    try { m_stdDev = std::stod(getParameter("StdDev").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StdDev', using default value."); }
    try { m_min = std::stod(getParameter("Min").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Min', using default value."); }
    try { m_max = std::stod(getParameter("Max").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Max', using default value."); }
    try { m_sensitivity = std::stod(getParameter("Sensitivity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Sensitivity', using default value."); }
    try { m_hilbertFilterLength = std::stoi(getParameter("HilbertFilterLength").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'HilbertFilterLength', using default value."); }

    SetParameters();

    double fc = 0.0;
    if (!UpdateCharacterizationFrequency(fc)) {
        return false;
    }
    if (fc == 0.0) {
        buildHilbert(m_hilbertFilterLength);
    }

    return true;
}

bool PhaseShifter_Block::UpdateCharacterizationFrequency(double& fc)
{
    fc = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    if (fc >= 0.0) {
        if (GetOutputPort(GetOutputPortName(0))->getCharacterizationFrequency() != fc) {
            GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fc);
        }
        return true;
    }

    return false;
}



PhaseShifter::QuantEnum PhaseShifter_Block::ConvertStringToQuantEnum(const std::string& value)
{
    const std::string key = NormalizeEnumString(value);
    if (key == "quant_no" || key == "no" || key == "0") {
        return PhaseShifter::Quant_NO;
    }
    if (key == "quant_bits" || key == "numberofbitsuniform" || key == "numberofbits" || key == "1") {
        return PhaseShifter::Quant_Bits;
    }
    if (key == "quant_custom" || key == "customlevels" || key == "2") {
        return PhaseShifter::Quant_Custom;
    }
    return PhaseShifter::Quant_NO;
}

PhaseShifter::ErrEnum PhaseShifter_Block::ConvertStringToErrEnum(const std::string& value)
{
    const std::string key = NormalizeEnumString(value);
    if (key == "err_none" || key == "none" || key == "0") {
        return PhaseShifter::Err_None;
    }
    if (key == "err_normal" || key == "normal" || key == "1") {
        return PhaseShifter::Err_Normal;
    }
    if (key == "err_uniform" || key == "uniform" || key == "2") {
        return PhaseShifter::Err_Uniform;
    }
    if (key == "err_custom" || key == "customerror" || key == "3") {
        return PhaseShifter::Err_Custom;
    }
    return PhaseShifter::Err_None;
}

void PhaseShifter_Block::buildHilbert(int Lin)
{
    m_L = Lin;
    if (m_L < 3) m_L = 3;
    if ((m_L % 2) == 0) ++m_L;
    m_h.assign(m_L, 0.0);

    const int M = (m_L - 1) / 2;
    for (int n = 0; n < m_L; ++n) {
        int m = n - M;
        if (m == 0) { m_h[n] = 0.0; continue; }
        if ((m & 1) != 0) {
            m_h[n] = 2.0 / (kPI * double(m));
        }
        else {
            m_h[n] = 0.0;
        }
    }
    m_x.clear();
    m_x.resize(m_L, 0.0);
}

double PhaseShifter_Block::hilbertConv() const
{
    double acc = 0.0;
    const int L = m_L;
    for (int k = 0; k < L; ++k) {
        const double xnk = m_x[L - 1 - k];
        acc += m_h[k] * xnk;
    }
    return acc;
}

double PhaseShifter_Block::delayedReal() const
{
    const int d = m_L / 2;
    const int idx = std::max(0, int(m_x.size()) - 1 - d);
    return m_x[idx];
}

double PhaseShifter_Block::computePhaseRad(double baseDeg)
{
    double D = baseDeg;

    if (m_quantization == PhaseShifter::Quant_Bits && m_numBits > 0) {
        const double step = 360.0 / double(1 << m_numBits);
        double q = D / step;
        double n = std::floor(q);
        double frac = q - n;
        if (frac > 0.5 - 1e-15) n += 1.0;
        D = n * step;
    }
    else if (m_quantization == PhaseShifter::Quant_Custom && m_levels.NumElements() > 0) {
        double best = m_levels(0);
        double bestDiff = std::fabs(D - best);
        for (int i = 1; i < (int)m_levels.NumElements(); ++i) {
            double v = m_levels(i);
            double diff = std::fabs(D - v);
            if (diff < bestDiff || (std::fabs(diff - bestDiff) < 1e-15 && v > best)) {
                best = v; bestDiff = diff;
            }
        }
        D = best;
    }

    if (m_phaseShiftError == PhaseShifter::Err_Normal) {
        std::normal_distribution<double> dist(0.0, std::fabs(m_stdDev));
        D += dist(m_rngN);
    }
    else if (m_phaseShiftError == PhaseShifter::Err_Uniform) {
        double a = std::min(m_min, m_max), b = std::max(m_min, m_max);
        std::uniform_real_distribution<double> dist(a, b);
        D += dist(m_rngU);
    }
    else if (m_phaseShiftError == PhaseShifter::Err_Custom) {
        D += m_customError;
    }

    return D * kPI / 180.0;
}

double PhaseShifter_Block::ampScale() const
{
    return std::pow(10.0, -m_insertionLoss / 20.0);
}


