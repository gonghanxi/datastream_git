#include "FreqMpyDiv_Block.h"
#include <complex>
#include <cmath>

FreqMpyDiv_Block::FreqMpyDiv_Block(const std::string& name)
    : Block(name)
{
}

void FreqMpyDiv_Block::SetDefaultParamters()
{
    m_multDiv = FreqMpyDiv::MD_Multiplier;
    m_operatorType = FreqMpyDiv::OP_PhaseOnly;
    m_nominalX = 1.0;
    m_maxX = 2.0;
    m_minX = 0.5;
    m_fcIn = 0.0;
    m_fcOut = 0.0;
}

void FreqMpyDiv_Block::SetParameters()
{
    if (m_freqMpyDiv) {
        m_freqMpyDiv->MultDiv = m_multDiv;
        m_freqMpyDiv->OperatorType = m_operatorType;
        m_freqMpyDiv->NominalX = m_nominalX;
        m_freqMpyDiv->MaxX = m_maxX;
        m_freqMpyDiv->MinX = m_minX;
    }
}

bool FreqMpyDiv_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool FreqMpyDiv_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool FreqMpyDiv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_freqMpyDiv = std::make_unique<FreqMpyDiv>();

    AddInputPort("input", m_freqMpyDiv->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("control", m_freqMpyDiv->control, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_freqMpyDiv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();

    try { m_multDiv = ConvertStringToMultDivEnum(getParameter("MultDiv").Value); } catch (...) {}
    try { m_nominalX = std::stod(getParameter("NominalX").Value); } catch (...) {}
    try { m_maxX = std::stod(getParameter("MaxX").Value); } catch (...) {}
    try { m_minX = std::stod(getParameter("MinX").Value); } catch (...) {}
    try { m_operatorType = ConvertStringToOperatorTypeEnum(getParameter("OperatorType").Value); } catch (...) {}

    m_freqMpyDiv->input.SetRate(1); // TODO: input not connected; timing setup may be unreliable
    m_freqMpyDiv->input.SetStartTime(getSimu().startTime); // TODO: input not connected; timing setup may be unreliable
    return true;
}

void FreqMpyDiv_Block::UpdateCharacterizationFrequency()
{
    if (m_freqMpyDiv) {
        m_freqMpyDiv->PropagateCharacterizationFrequency();
    }
}

bool FreqMpyDiv_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string controlPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    UpdateCharacterizationFrequency();

    bool controlConnected = GetInputPort(controlPort)->IsConnected();
    std::vector<SystemVueModelBuilder::EnvelopeSignal> controlData;
    if (controlConnected) {
        controlData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(controlPort);
    }

    double fcIn = GetInputPort(inputPort)->getCharacterizationFrequency();
    m_fcIn = fcIn;

    if (m_minX <= 0.0) m_minX = 1e-6;
    if (m_maxX < m_minX) m_maxX = m_minX;
    double xNom = std::min(std::max(m_nominalX, m_minX), m_maxX);
    double fcOut = (m_multDiv == FreqMpyDiv::MD_Multiplier) ? (fcIn * xNom) : (xNom > 0.0 ? fcIn / xNom : 0.0);
    m_fcOut = fcOut;

    if (GetOutputPort(outputPort)->getCharacterizationFrequency() != fcOut) {
        GetOutputPort(outputPort)->setCharacterizationFrequency(fcOut);
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    const SimuParameter simulator_param = getSimu();
    const double tNow = (simulator_param.samplingRate > 0.0)
            ? (simulator_param.startTime + static_cast<double>(GetCount()) / simulator_param.samplingRate)
            : 0.0;

    for (size_t i = 0; i < inputData.size(); ++i) {
        const auto& xin = inputData[i];
        double ctrl = (controlConnected && i < controlData.size()) ? controlData[i].real() : 0.0;

        double X = std::min(std::max(m_nominalX + ctrl, m_minX), m_maxX);

        const std::complex<double> cx = xin.complex();
        const double r = std::abs(cx);
        const double th = std::atan2(cx.imag(), cx.real());

        const double Xeff_phase = (m_multDiv == FreqMpyDiv::MD_Multiplier) ? X : (1.0 / X);
        const double amp_exp = (m_operatorType == FreqMpyDiv::OP_PhaseOnly) ? 1.0 : Xeff_phase;

        std::complex<double> y = std::polar(std::pow(r, amp_exp), Xeff_phase * th);

        const double dFc = ((m_multDiv == FreqMpyDiv::MD_Multiplier) ? (fcIn * X) : (X > 0.0 ? fcIn / X : 0.0)) - fcOut;
        if (dFc != 0.0 && tNow != 0.0) {
            const double phi = 2.0 * M_PI * dFc * tNow;
            const std::complex<double> rot(std::cos(phi), std::sin(phi));
            y *= rot;
        }

        SystemVueModelBuilder::EnvelopeSignal outSig;
        CopyToEnvelopeSignal(y, outSig);
        outputData.push_back(outSig);
    }

    WriteOutputData(outputPort, outputData);
    Advance();

    return true;
}

bool FreqMpyDiv_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string controlPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    bool controlConnected = GetInputPort(controlPort)->IsConnected();
    std::vector<SystemVueModelBuilder::EnvelopeSignal> controlData;
    if (controlConnected) {
        controlData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(controlPort);
        m_controlBuffer.push_back(controlData[0]);
    }

    double fcIn = GetInputPort(inputPort)->getCharacterizationFrequency();
    m_fcIn = fcIn;

    if (m_minX <= 0.0) m_minX = 1e-6;
    if (m_maxX < m_minX) m_maxX = m_minX;
    double xNom = std::min(std::max(m_nominalX, m_minX), m_maxX);
    double fcOut = (m_multDiv == FreqMpyDiv::MD_Multiplier) ? (fcIn * xNom) : (xNom > 0.0 ? fcIn / xNom : 0.0);
    m_fcOut = fcOut;

    if (GetOutputPort(outputPort)->getCharacterizationFrequency() != fcOut) {
        GetOutputPort(outputPort)->setCharacterizationFrequency(fcOut);
    }

    bool CanProcessData = false;
    m_inputBuffer.push_back(inputData[0]);
    if(controlConnected) {
        if(m_inputBuffer.size() >= 1 && m_controlBuffer.size() >= 1) {
            CanProcessData = true;
        }
    }
    else {
        if(m_inputBuffer.size() >= 1) {
            CanProcessData = true;
        }
    }
    if(CanProcessData) {
        std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
        outputData.reserve(inputData.size());

        const SimuParameter simulator_param = getSimu();
        const double tNow = (simulator_param.samplingRate > 0.0)
                ? (simulator_param.startTime + static_cast<double>(GetCount()) / simulator_param.samplingRate)
                : 0.0;

        for (size_t i = 0; i < inputData.size(); ++i) {
            const auto& xin = m_inputBuffer[i];
            double ctrl = (controlConnected && i < m_controlBuffer.size()) ? m_controlBuffer[i].real() : 0.0;

            double X = std::min(std::max(m_nominalX + ctrl, m_minX), m_maxX);

            const std::complex<double> cx = xin.complex();
            const double r = std::abs(cx);
            const double th = std::atan2(cx.imag(), cx.real());

            const double Xeff_phase = (m_multDiv == FreqMpyDiv::MD_Multiplier) ? X : (1.0 / X);
            const double amp_exp = (m_operatorType == FreqMpyDiv::OP_PhaseOnly) ? 1.0 : Xeff_phase;

            std::complex<double> y = std::polar(std::pow(r, amp_exp), Xeff_phase * th);

            const double dFc = ((m_multDiv == FreqMpyDiv::MD_Multiplier) ? (fcIn * X) : (X > 0.0 ? fcIn / X : 0.0)) - fcOut;
            if (dFc != 0.0 && tNow != 0.0) {
                const double phi = 2.0 * M_PI * dFc * tNow;
                const std::complex<double> rot(std::cos(phi), std::sin(phi));
                y *= rot;
            }

            SystemVueModelBuilder::EnvelopeSignal outSig;
            CopyToEnvelopeSignal(y, outSig);
            outputData.push_back(outSig);
        }
        for (const auto& val : outputData)
        {
            m_outputQueue.push(val);
        }
        if (!m_outputQueue.empty())
        {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[DeMux_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}

FreqMpyDiv::MultDivEnum FreqMpyDiv_Block::ConvertStringToMultDivEnum(const std::string& value)
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

    if (lowerValue == "md_multiplier") {
        return FreqMpyDiv::MD_Multiplier;
    } else if (lowerValue == "md_divider") {
        return FreqMpyDiv::MD_Divider;
    }

    if (lowerValue == "multiplier" || lowerValue == "0") {
        return FreqMpyDiv::MD_Multiplier;
    } else if (lowerValue == "divider" || lowerValue == "1") {
        return FreqMpyDiv::MD_Divider;
    }
    return FreqMpyDiv::MD_Multiplier;
}

FreqMpyDiv::OperatorTypeEnum FreqMpyDiv_Block::ConvertStringToOperatorTypeEnum(const std::string& value)
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

    if (lowerValue == "op_full") {
        return FreqMpyDiv::OP_Full;
    } else if (lowerValue == "op_phaseonly") {
        return FreqMpyDiv::OP_PhaseOnly;
    }
    return FreqMpyDiv::OP_PhaseOnly;
}







