#include "Modulator_Block.h"

Modulator_Block::Modulator_Block(const std::string &name)
    :Block(name)
{

}

bool Modulator_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    while(!m_quad_outputQueue.empty()) m_quad_outputQueue.pop();
    return true;
}

bool Modulator_Block::DataStreamRun()
{
    if (!CanProcess()) {
        return false;
    }

    // 1. 获取时间戳
    double tNow = m_modulator->output.GetTime(0, 1);  // 需要实现获取当前时间的方法
    double dt = 0.0;
    if (std::isfinite(m_lastTime)) {
        dt = std::max(0.0, tNow - m_lastTime);
    }
    m_lastTime = tNow;

    // 2. 读取输入数据
    // 检查哪些输入端口连接了
    bool input1Connected = GetInputPort("input1")->IsConnected();
    bool input2Connected = GetInputPort("input2")->IsConnected();
    bool LOConnected = GetInputPort("LO")->IsConnected();

    // 读取input1数据（如果是振幅类型且未连接，使用默认值）
    double x1 = 0.0;
    if (input1Connected) {
        auto input1Data = ReadInputData<double>("input1");
        if (!input1Data.empty()) {
            x1 = input1Data[0];
        }
    } else if (m_InputType != Modulator::InIQ) {
        x1 = (m_InputType == Modulator::InIQ ? 0.0 : 1.0);  // 振幅类型默认值为1
    }

    // 读取input2数据
    double x2 = 0.0;
    if (input2Connected) {
        auto input2Data = ReadInputData<double>("input2");
        if (!input2Data.empty()) {
            x2 = input2Data[0];
        }
    }

    // 读取LO数据（EnvelopeSignal类型）
    std::complex<double> LOValue(0.0, 0.0);
    double LO_fc = 0.0;
    if (LOConnected) {
        auto LOData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>("LO");
        if (!LOData.empty()) {
            LOValue = LOData[0].complex();
            // 获取LO的频率信息
            // 注意：需要从BufferReader中获取表征频率
            LO_fc = GetInputPort("LO")->getCharacterizationFrequency();
        }
    }

    // 3. 数据处理逻辑
    bool isAmpType = (m_InputType != Modulator::InIQ);
    double Sa_eff = m_AmpSensitivity;
    if (!input1Connected && isAmpType) {
        Sa_eff = 1.0;  // 振幅输入未连接时，灵敏度为1
    }

    std::complex<double> cx(0.0, 0.0);
    double th0 = m_InitialPhase * M_PI / 180.0;  // 角度转弧度

    // 根据输入类型处理
    if (m_InputType == Modulator::InIQ) {
        // I/Q输入
        cx = std::complex<double>(x1, x2) * m_AmpSensitivity;
    } else if (m_InputType == Modulator::InAmpPhase) {
        // 振幅/相位输入
        double phi = th0 + (m_PhaseSensitivity * M_PI / 180.0) * x2;  // 相位灵敏度
        double A = Sa_eff * x1;
        cx = std::polar(A, phi);
    } else if (m_InputType == Modulator::InAmpFreq) {
        // 振幅/频率输入
        static double phaseAcc = 0.0;
        phaseAcc += 2.0 * M_PI * m_FreqSensitivity * x2 * dt;
        double A = Sa_eff * x1;
        cx = std::polar(A, th0 + phaseAcc);
    }

    // 如果LO输入连接了，乘以LO值
    if (LOConnected) {
        cx *= LOValue;
    }

    // 镜像信号处理
    if (m_MirrorSignal == Modulator::Mirror_Yes) {
        cx = std::conj(cx);
    }

    // IQ损伤处理
    if (m_ShowIQ_Impairments == Modulator::ShowIQ_YES) {
        double gI = std::pow(10.0, (+0.5 * m_GainImbalance) / 20.0);
        double gQ = std::pow(10.0, (-0.5 * m_GainImbalance) / 20.0);
        double phiI = (-m_PhaseImbalance * M_PI / 180.0) * 0.5;
        double phiQ = (+m_PhaseImbalance * M_PI / 180.0) * 0.5;

        double I = cx.real();
        double Q = cx.imag();
        double Ip = gI * I * std::cos(phiI) - gQ * Q * std::sin(phiQ);
        double Qp = gI * I * std::sin(phiI) + gQ * Q * std::cos(phiQ);

        std::complex<double> cx_imp(Ip, Qp);
        cx_imp *= std::exp(std::complex<double>(0.0, m_IQ_Rotation * M_PI / 180.0));
        cx_imp += std::complex<double>(m_I_OriginOffset, m_Q_OriginOffset);

        cx = cx_imp;
    }

    // 计算正交输出
    std::complex<double> quad;
    if (m_ConjugatedQuadrature == Modulator::CQ_No) {
        quad = std::complex<double>(-cx.imag(), cx.real());
    } else {
        quad = std::complex<double>(cx.imag(), -cx.real());
    }

    // 4. 写入输出数据
    // 获取输出频率
    double outputFc = LOConnected ? LO_fc : m_FCarrier;

    // 设置输出频率
    GetOutputPort("output")->setCharacterizationFrequency(outputFc);
    GetOutputPort("quad_output")->setCharacterizationFrequency(outputFc);

    // 写入主输出
    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(cx));
    WriteOutputData("output", outputData);

    // 写入正交输出
    std::vector<SystemVueModelBuilder::EnvelopeSignal> quadOutputData;
    quadOutputData.push_back(SystemVueModelBuilder::EnvelopeSignal(quad));
    WriteOutputData("quad_output", quadOutputData);

    return true;
}

bool Modulator_Block::TimeDrivenRun()
{
    // 1. 获取时间戳
    double tNow = m_modulator->output.GetTime(0, 1);
    double dt = 0.0;
    if (std::isfinite(m_lastTime)) {
        dt = std::max(0.0, tNow - m_lastTime);
    }
    m_lastTime = tNow;

    // 2. 读取输入数据
    bool input1Connected = GetInputPort("input1")->IsConnected();
    bool input2Connected = GetInputPort("input2")->IsConnected();
    bool LOConnected = GetInputPort("LO")->IsConnected();

    bool allInputsReady = true;

    // 读取input1数据
    double x1 = 0.0;
    if (input1Connected) {
        auto input1Data = ReadInputData<double>("input1");
        if (!input1Data.empty()) {
            x1 = input1Data[0];
        }
        else {
            allInputsReady = false;
        }
    } else if (m_InputType != Modulator::InIQ) {
        x1 = (m_InputType == Modulator::InIQ ? 0.0 : 1.0);
    }
    m_input1Buffer.push_back(x1);

    // 读取input2数据
    double x2 = 0.0;
    if (input2Connected) {
        auto input2Data = ReadInputData<double>("input2");
        if (!input2Data.empty()) {
            x2 = input2Data[0];
        }
        else {
            allInputsReady = false;
        }
    }
    m_input2Buffer.push_back(x2);

    // 读取LO数据
    std::complex<double> LOValue(0.0, 0.0);
    double LO_fc = 0.0;
    if (LOConnected) {
        auto LOData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>("LO");
        if (!LOData.empty()) {
            LOValue = LOData[0].complex();
            LO_fc = GetInputPort("LO")->getCharacterizationFrequency();
        }
        else {
            allInputsReady = false;
        }
    }
    m_loBuffer.push_back(LOValue);

    // 判断是否可以处理数据
    bool CanProcessData = allInputsReady;

    if (!CanProcessData) {
        return true;  // 数据不足，等待下一次调用
    }

    // 3. 如果所有连接端口都有数据，进行实际处理
    // 3. 数据处理逻辑
    bool isAmpType = (m_InputType != Modulator::InIQ);
    double Sa_eff = m_AmpSensitivity;
    if (!input1Connected && isAmpType) {
        Sa_eff = 1.0;  // 振幅输入未连接时，灵敏度为1
    }

    std::complex<double> cx(0.0, 0.0);
    double th0 = m_InitialPhase * M_PI / 180.0;  // 角度转弧度

    // 根据输入类型处理
    if (m_InputType == Modulator::InIQ) {
        // I/Q输入
        cx = std::complex<double>(x1, x2) * m_AmpSensitivity;
    } else if (m_InputType == Modulator::InAmpPhase) {
        // 振幅/相位输入
        double phi = th0 + (m_PhaseSensitivity * M_PI / 180.0) * x2;  // 相位灵敏度
        double A = Sa_eff * x1;
        cx = std::polar(A, phi);
    } else if (m_InputType == Modulator::InAmpFreq) {
        // 振幅/频率输入
        static double phaseAcc = 0.0;
        phaseAcc += 2.0 * M_PI * m_FreqSensitivity * x2 * dt;
        double A = Sa_eff * x1;
        cx = std::polar(A, th0 + phaseAcc);
    }

    // 如果LO输入连接了，乘以LO值
    if (LOConnected) {
        cx *= LOValue;
    }

    // 镜像信号处理
    if (m_MirrorSignal == Modulator::Mirror_Yes) {
        cx = std::conj(cx);
    }

    // IQ损伤处理
    if (m_ShowIQ_Impairments == Modulator::ShowIQ_YES) {
        double gI = std::pow(10.0, (+0.5 * m_GainImbalance) / 20.0);
        double gQ = std::pow(10.0, (-0.5 * m_GainImbalance) / 20.0);
        double phiI = (-m_PhaseImbalance * M_PI / 180.0) * 0.5;
        double phiQ = (+m_PhaseImbalance * M_PI / 180.0) * 0.5;

        double I = cx.real();
        double Q = cx.imag();
        double Ip = gI * I * std::cos(phiI) - gQ * Q * std::sin(phiQ);
        double Qp = gI * I * std::sin(phiI) + gQ * Q * std::cos(phiQ);

        std::complex<double> cx_imp(Ip, Qp);
        cx_imp *= std::exp(std::complex<double>(0.0, m_IQ_Rotation * M_PI / 180.0));
        cx_imp += std::complex<double>(m_I_OriginOffset, m_Q_OriginOffset);

        cx = cx_imp;
    }

    // 计算正交输出
    std::complex<double> quad;
    if (m_ConjugatedQuadrature == Modulator::CQ_No) {
        quad = std::complex<double>(-cx.imag(), cx.real());
    } else {
        quad = std::complex<double>(cx.imag(), -cx.real());
    }

    // 4. 写入输出数据
    // 获取输出频率
    double outputFc = LOConnected ? LO_fc : m_FCarrier;

    // 设置输出频率
    GetOutputPort("output")->setCharacterizationFrequency(outputFc);
    GetOutputPort("quad_output")->setCharacterizationFrequency(outputFc);

    // 写入主输出
    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(cx));
    WriteOutputData("output", outputData);

    // 写入正交输出
    std::vector<SystemVueModelBuilder::EnvelopeSignal> quadOutputData;
    quadOutputData.push_back(SystemVueModelBuilder::EnvelopeSignal(quad));
    WriteOutputData("quad_output", quadOutputData);

    return true;
}
bool Modulator_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Modulator_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_modulator = std::make_unique<Modulator>();

    AddInputPort("input1", m_modulator->input1, 1, DataType::TIMED_DOUBLE);
    AddInputPort("input2", m_modulator->input2, 1, DataType::TIMED_DOUBLE);
    AddInputPort("LO", m_modulator->LO, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_modulator->output, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("quad_output", m_modulator->quad_output, 1, DataType::ENVELOPE_SIGNAL);


    simulator_param = getSimu();
    SetDefaultParameters();

    try { m_InputType = ConvertStringToInputTypeEnum(getParameter("InputType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InputType', using default value."); }
    try { m_FCarrier = std::stod(getParameter("FCarrier").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FCarrier', using default value."); }
    try { m_InitialPhase = std::stod(getParameter("InitialPhase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialPhase', using default value."); }
    try { m_AmpSensitivity = std::stod(getParameter("AmpSensitivity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AmpSensitivity', using default value."); }
    try { m_PhaseSensitivity = std::stod(getParameter("PhaseSensitivity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseSensitivity', using default value."); }
    try { m_FreqSensitivity = std::stod(getParameter("FreqSensitivity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FreqSensitivity', using default value."); }
    try { m_ConjugatedQuadrature = ConvertStringToConjQuadEnum(getParameter("ConjugatedQuadrature").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ConjugatedQuadrature', using default value."); }
    try { m_MirrorSignal = ConvertStringToMirrorEnum(getParameter("MirrorSignal").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'MirrorSignal', using default value."); }
    try { m_ShowIQ_Impairments = ConvertStringToShowIQEnum(getParameter("ShowIQ_Impairments").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowIQ_Impairments', using default value."); }
    try { m_GainImbalance = std::stod(getParameter("GainImbalance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'GainImbalance', using default value."); }
    try { m_PhaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseImbalance', using default value."); }
    try { m_I_OriginOffset = std::stod(getParameter("I_OriginOffset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'I_OriginOffset', using default value."); }
    try { m_Q_OriginOffset = std::stod(getParameter("Q_OriginOffset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Q_OriginOffset', using default value."); }
    try { m_IQ_Rotation = std::stod(getParameter("IQ_Rotation").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IQ_Rotation', using default value."); }


    SetParameters(m_InputType, m_FCarrier,m_InitialPhase, m_AmpSensitivity,
                  m_PhaseSensitivity, m_FreqSensitivity, m_ConjugatedQuadrature,
                  m_MirrorSignal, m_ShowIQ_Impairments, m_GainImbalance,
                  m_PhaseImbalance,m_I_OriginOffset, m_Q_OriginOffset,m_IQ_Rotation);

    return true;
}

void Modulator_Block::SetParameters(Modulator::InputTypeEnum inputype, double fcarrier,
                                    double initialphase, double ampsensitivity,
                                    double phasesensitivity, double freqsensitivity,
                                    Modulator::ConjQuadEnum conjugatedquadrature, Modulator::MirrorEnum mirrorsignal,
                                    Modulator::ShowIQEnum showiq_impairments, double gainimbalance,
                                    double phaseimbalance, double i_originoffset,
                                    double q_originoffset, double iq_rotation)
{
    if(m_modulator) {
        m_modulator->InputType = inputype;
        m_modulator->FCarrier = fcarrier;
        m_modulator->InitialPhase = initialphase;
        m_modulator->AmpSensitivity = ampsensitivity;
        m_modulator->PhaseSensitivity = phasesensitivity;
        m_modulator->FreqSensitivity = freqsensitivity;
        m_modulator->ConjugatedQuadrature = conjugatedquadrature;
        m_modulator->MirrorSignal = mirrorsignal;
        m_modulator->ShowIQ_Impairments = showiq_impairments;
        m_modulator->GainImbalance = gainimbalance;
        m_modulator->PhaseImbalance = phaseimbalance;
        m_modulator->I_OriginOffset = i_originoffset;
        m_modulator->Q_OriginOffset = q_originoffset;
        m_modulator->IQ_Rotation = iq_rotation;
        m_modulator->output.SetSampleRate(simulator_param.samplingRate);
        m_modulator->quad_output.SetSampleRate(simulator_param.samplingRate);
    }
}

Modulator::InputTypeEnum Modulator_Block::ConvertStringToInputTypeEnum(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "iniq" || lowerValue == "0") {
        return Modulator::InIQ;
    } else if (lowerValue == "inampphase" || lowerValue == "1") {
        return Modulator::InAmpPhase;
    } else if (lowerValue == "inampfreq" || lowerValue == "2") {
        return Modulator::InAmpFreq;
    }
    return Modulator::InIQ;
}

Modulator::ConjQuadEnum Modulator_Block::ConvertStringToConjQuadEnum(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "cq_no" || lowerValue == "0") {
        return Modulator::CQ_No;
    } else if (lowerValue == "cq_yes" || lowerValue == "1") {
        return Modulator::CQ_Yes;
    }
    return Modulator::CQ_No;
}

Modulator::MirrorEnum Modulator_Block::ConvertStringToMirrorEnum(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "mirror_no" || lowerValue == "0") {
        return Modulator::Mirror_No;
    } else if (lowerValue == "mirror_yes" || lowerValue == "1") {
        return Modulator::Mirror_Yes;
    }
    return Modulator::Mirror_No;
}

Modulator::ShowIQEnum Modulator_Block::ConvertStringToShowIQEnum(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "showiq_no" || lowerValue == "0") {
        return Modulator::ShowIQ_NO;
    } else if (lowerValue == "showiq_yes" || lowerValue == "1") {
        return Modulator::ShowIQ_YES;
    }
    return Modulator::ShowIQ_NO;
}

void Modulator_Block::PropagateCharacterizationFrequency()
{
    double fc = m_FCarrier;
    const std::string loPortName = GetInputPortName(2);
    if (GetInputPort(loPortName)) {
        fc = GetInputPort(loPortName)->getCharacterizationFrequency();
    }


    if (fc >= 0.0) {
        GetOutputPort("output")->setCharacterizationFrequency(fc);
        GetOutputPort("quad_output")->setCharacterizationFrequency(fc);
    }
    else {
        return;
    }

}



void Modulator_Block::SetDefaultParameters()
{
    m_InputType = Modulator::InIQ;
    m_FCarrier = 0.2e6;
    m_InitialPhase = 0;
    m_AmpSensitivity = 1;
    m_PhaseSensitivity = 90;
    m_FreqSensitivity = 10000;
    m_ConjugatedQuadrature = Modulator::CQ_No;
    m_MirrorSignal = Modulator::Mirror_No;
    m_ShowIQ_Impairments = Modulator::ShowIQ_NO;
    m_GainImbalance = 0.0;
    m_PhaseImbalance = 0.0;
    m_I_OriginOffset = 0.0;
    m_Q_OriginOffset = 0.0;
    m_IQ_Rotation = 0.0;
}
