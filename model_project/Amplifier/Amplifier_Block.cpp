#include "Amplifier_Block.h"
#include "DataTypesAndParsers.h"

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

Amplifier_Block::Amplifier_Block(const std::string& name)
    : Block(name)
{
}

void Amplifier_Block::SetDefaultParamters()
{
    m_gainUnit = Amplifier::voltage;
    m_gain = 1.0;

    m_quantization = Amplifier::NO;
    m_numBits = 6;
    m_stepSize = 0.5;
    m_maxGain = 10.0;

    m_levels.Resize(1, 3);
    m_levels(0,0) = 0;
    m_levels(0,1) = 0;
    m_levels(0,2) = 0;

    m_gainError = Amplifier::None;
    m_stdDev = 0.5;
    m_min = -0.5;
    m_max = 0.5;
    m_customError = 0.0;

    m_noiseFigure = 0.0;
    m_gcType = Amplifier::none;

    TOIout = 0.1;
    dBc1out = 0.01;
    PSat = 0.032;
    GCSat = 3;
    RappS = 3;

    GComp.Resize(1,3);
    GComp(0,0) = 0;
    GComp(0,1) = 0;
    GComp(0,2) = 0;


    RefR = 50.0;
}

void Amplifier_Block::SetParameters()
{
    if (!m_amplifier) {
        return;
    }

    m_amplifier->GainUnit = m_gainUnit;
    m_amplifier->Gain = m_gain;

    m_amplifier->Quantization = m_quantization;
    m_amplifier->NumBits = m_numBits;
    m_amplifier->StepSize = m_stepSize;
    m_amplifier->MaxGain = m_maxGain;
    m_amplifier->Levels = m_levels;

    m_amplifier->GainError = m_gainError;
    m_amplifier->StdDev = m_stdDev;
    m_amplifier->Min = m_min;
    m_amplifier->Max = m_max;
    m_amplifier->CustomError = m_customError;

    m_amplifier->NoiseFigure = m_noiseFigure;
    m_amplifier->GCType = m_gcType;


    m_amplifier->TOIout = TOIout;
    m_amplifier->dBc1out = dBc1out;
    m_amplifier->PSat = PSat;
    m_amplifier->GCSat = GCSat;
    m_amplifier->RappS = RappS;
    m_amplifier->GComp = GComp;
    m_amplifier->RefR = RefR;
}

bool Amplifier_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Amplifier_Block::UpdateCharacterizationFrequency(double& fc)
{
    fc = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    if (fc >= 0.0) {
        auto* outPort = GetOutputPort(GetOutputPortName(0));
        if (outPort && outPort->getCharacterizationFrequency() != fc) {
            outPort->setCharacterizationFrequency(fc);
        }
        return true;
    }

    std::cout << "characterization frequency must be >= 0." << std::endl;
    return false;
}

bool Amplifier_Block::Run()
{
    using SystemVueModelBuilder::EnvelopeSignal;

    const std::string inputPort = GetInputPortName(0);
    const std::string controlPort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    BufferReader* input = GetInputPort(inputPort);

    // 强制驱动时间轴，保证 Run 阶段能拿到有效 SampleRate / TimeStep
//	(void)input.GetTime(0, GetCount());

    const double t = simulator_param.startTime + static_cast<double>(m_amplifier->GetCount()) / simulator_param.samplingRate;


    // ===== 1. 读取输入 =====
    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    EnvelopeSignal xin = inputData[0];

    const double fc = input->getCharacterizationFrequency();

    if (!m_amplifier->updateNoiseSigmaIfNeeded(fc)) {
        return false;
    }

    // ===== 2. 先加输入端噪声 =====
    EnvelopeSignal x2 = m_amplifier->addInputNoise(xin, fc);

    // ===== 3. 获取增益源 =====
    double gainSrc = m_gain;
    const bool hasControl = GetInputPort(GetInputPortName(1))->IsConnected();
    if (hasControl) {
        auto controlData = ReadInputData<double>(controlPort);
        if (!controlData.empty()) {
            gainSrc = controlData[0];
        }
    }

    // ===== 4. 计算小信号增益 =====
    double gainDb = 0.0;
    const double c1 = m_amplifier->computeSmallSignalGainLin(gainSrc, gainDb);

    // ===== 5. 按 GCType 处理 =====
    EnvelopeSignal yout;

    if (m_gcType == Amplifier::GCTypeEnum::none) {
        if (fc > 0.0) {
            yout = EnvelopeSignal(x2.complex() * c1);
        }
        else {
            yout = EnvelopeSignal(x2.real() * c1);
        }
    }
    else {
        const double ain = m_amplifier->getInputAmplitude(x2, fc);
        double aout = c1 * ain;

        switch (m_gcType) {
        case Amplifier::GCTypeEnum::TOI:
            aout = m_amplifier->applyTOI(ain, c1);
            break;

        case Amplifier::GCTypeEnum::dBc1:
            aout = m_amplifier->applydBc1(ain, c1);
            break;

        case Amplifier::GCTypeEnum::TOI_dBc1:
            aout = m_amplifier->applyTOIdBc1(ain, c1);
            break;

        case Amplifier::GCTypeEnum::PSat_GCSat_TOI:
        case Amplifier::GCTypeEnum::PSat_GCSat_dBc1:
        case Amplifier::GCTypeEnum::PSat_GCSat_TOI_dBc1:
            aout = m_amplifier->applyPSatGCSat(ain, c1, fc);
            break;

        case Amplifier::GCTypeEnum::RappNonlinearity:
            aout = m_amplifier->applyRapp(ain, c1);
            break;

        case Amplifier::GCTypeEnum::Gain_compression_vs_input_power:
            // 黑盒确认：PC 第三列不作用输出相位，只做幅度压缩
            aout = m_amplifier->tableOutputAmplitude(ain, c1, gainDb, m_amplifier->gcompTable_);
            break;

        case Amplifier::GCTypeEnum::AM_AM_and_AMPM_vs_input_power:
            // 黑盒确认：合法 AMPM 表中 AM2PM 不作用输出相位，只用 AM2AM 递推得到幅度压缩
            aout = m_amplifier->tableOutputAmplitude(ain, c1, gainDb, m_amplifier->amamTable_);
            break;

        default:
            aout = c1 * ain;
            break;
        }

        yout = m_amplifier->makeOutputWithAmplitude(x2, fc, aout);
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData(inputData.size());
    outputData[0U] = yout;
    WriteOutputData(outputPort, outputData);

    // 推进算法内部计数器（原算法 TimedDFModel 框架自动调用）
    m_amplifier->Advance();

    return true;
}

bool Amplifier_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_amplifier = std::make_unique<Amplifier>();

    AddInputPort("input", m_amplifier->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("control", m_amplifier->control, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_amplifier->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_gainUnit = ConvertStringToGainUnit(getParameter("GainUnit").Value); } catch (...) { }
    try { m_gain = std::stod(getParameter("Gain").Value); } catch (...) { }
    try { m_quantization = ConvertStringToQuantization(getParameter("Quantization").Value); } catch (...) { }
    try { m_numBits = std::stoi(getParameter("NumBits").Value); } catch (...) { }
    try { m_stepSize = std::stod(getParameter("StepSize").Value); } catch (...) { }
    try { m_maxGain = std::stod(getParameter("MaxGain").Value); } catch (...) { }
    try { m_levels = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("Levels").Value); } catch (...) { }
    try { m_gainError = ConvertStringToGainError(getParameter("GainError").Value); } catch (...) { }
    try { m_stdDev = std::stod(getParameter("StdDev").Value); } catch (...) { }
    try { m_min = std::stod(getParameter("Min").Value); } catch (...) { }
    try { m_max = std::stod(getParameter("Max").Value); } catch (...) { }
    try { m_customError = std::stod(getParameter("CustomError").Value); } catch (...) { }
    try { m_noiseFigure = std::stod(getParameter("NoiseFigure").Value); } catch (...) { }
    try { m_gcType = ConvertStringToGCType(getParameter("GCType").Value); } catch (...) { }

    try { TOIout = std::stod(getParameter("TOIout").Value); } catch (...) { }
    try { dBc1out = std::stod(getParameter("dBc1out").Value); } catch (...) { }
    try { PSat = std::stod(getParameter("PSat").Value); } catch (...) { }
    try { GCSat = std::stod(getParameter("GCSat").Value); } catch (...) { }
    try { RappS = std::stoi(getParameter("RappS").Value); } catch (...) { }
    try { GComp = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("GComp").Value); } catch (...) { }

    try { RefR = std::stod(getParameter("RefR").Value); } catch (...) { }

    if (m_numBits < 0) {
        m_numBits = 0;
    }

    SetParameters();

    if(!m_amplifier->Setup()) return false;
    m_amplifier->input.SetSampleRate(simulator_param.samplingRate);
    m_amplifier->input.SetTimeStep(simulator_param.time_Interval);

    m_amplifier->input.ResizeMemory(1, true);
    m_amplifier->control.ResizeMemory(1, true);
    m_amplifier->output.ResizeMemory(1, true);

    return true;
}

Amplifier::GainUnitEnum Amplifier_Block::ConvertStringToGainUnit(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "voltage" || lower == "0") {
        return Amplifier::voltage;
    }
    if (lower == "db" || lower == "1") {
        return Amplifier::dB;
    }
    return Amplifier::voltage;
}

Amplifier::QuantizationEnum Amplifier_Block::ConvertStringToQuantization(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return Amplifier::NO;
    }
    if (lower == "number_of_bits_uniform" || lower == "1") {
        return Amplifier::Number_of_Bits_Uniform;
    }
    if (lower == "custom_levels" || lower == "2") {
        return Amplifier::Custom_Levels;
    }
    return Amplifier::NO;
}

Amplifier::GainErrorEnum Amplifier_Block::ConvertStringToGainError(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "none" || lower == "0") {
        return Amplifier::None;
    }
    if (lower == "normal" || lower == "1") {
        return Amplifier::Normal;
    }
    if (lower == "uniform" || lower == "2") {
        return Amplifier::Uniform;
    }
    if (lower == "custom_error" || lower == "3") {
        return Amplifier::Custom_Error;
    }
    return Amplifier::None;
}

Amplifier::GCTypeEnum Amplifier_Block::ConvertStringToGCType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "none" || lower == "0") {
        return Amplifier::none;
    }
    if (lower == "toi" || lower == "1") {
        return Amplifier::TOI;
    }
    if (lower == "dbc1" || lower == "2") {
        return Amplifier::dBc1;
    }
    if (lower == "toi_dbc1" || lower == "3") {
        return Amplifier::TOI_dBc1;
    }
    if (lower == "psat_gcsat_toi" || lower == "4") {
        return Amplifier::PSat_GCSat_TOI;
    }
    if (lower == "psat_gcsat_dbc1" || lower == "5") {
        return Amplifier::PSat_GCSat_dBc1;
    }
    if (lower == "psat_gcsat_toi_dbc1" || lower == "6") {
        return Amplifier::PSat_GCSat_TOI_dBc1;
    }
    if (lower == "rappnonlinearity" || lower == "7") {
        return Amplifier::RappNonlinearity;
    }
    if (lower == "gain_compression_vs_input_power" || lower == "8") {
        return Amplifier::Gain_compression_vs_input_power;
    }
    if (lower == "am_am_and_ampm_vs_input_power" || lower == "9") {
        return Amplifier::AM_AM_and_AMPM_vs_input_power;
    }
    return Amplifier::none;
}


