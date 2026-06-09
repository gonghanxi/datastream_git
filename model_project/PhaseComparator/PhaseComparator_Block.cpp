#include "PhaseComparator_Block.h"
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

static const double PC_PI = 3.14159265358979323846;
static const double PC_TWO_PI = 2.0 * PC_PI;

PhaseComparator_Block::PhaseComparator_Block(const std::string &name)
    :Block(name)
{

}

bool PhaseComparator_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    if (!std::isfinite(GainConstant))
    {
        LOG_ERROR("PhaseComparator: GainConstant must be finite.");
        return false;
    }

    if (MaxAngle < 0.0)
    {
        LOG_ERROR("PhaseComparator: MaxAngle must be >= 0.");
        return false;
    }
    return true;
}

bool PhaseComparator_Block::DataStreamRun()
{
    BufferReader* s1 = GetInputPort("s1");
    BufferReader* s2 = GetInputPort("s2");
    Buffer* output = GetOutputPort("output");
    auto s1Data = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    auto s2Data = ReadInputData<EnvelopeSignal>(GetInputPortName(1));
    std::vector<EnvelopeSignal> outputData(1);
    const double t = simulator_param.startTime + static_cast<double>(m_phase->GetCount()) / simulator_param.samplingRate;

    const double fc1 = s1->getCharacterizationFrequency();
    const double fc2 = s2->getCharacterizationFrequency();

    if (fc1 <= 0.0 || fc2 <= 0.0)
    {
        LOG_ERROR("PhaseComparator: inputs must be envelope signals with characterization frequency > 0.");
    }

    m_phase->fcOut_ = fc1;
    output->setCharacterizationFrequency(m_phase->fcOut_);

    const double fcOut = m_phase->fcOut_;

    const std::complex<double> x1 =
        s1Data[0].ConvertToNewFc(fc1, fcOut, t);
    const std::complex<double> x2 =
        s2Data[0].ConvertToNewFc(fc2, fcOut, t);

    const std::complex<double> z = x1 * std::conj(x2);
    const double dTheta = std::atan2(z.imag(), z.real());

    const double scaleRad2VoltDeg = GainConstant * (180.0 / PC_PI);

    double outVal = 0.0;

    switch (PhaseCharacteristicType)
    {
    case PhaseComparator::PhaseFreq:
    {
        if (MaxAngle <= 0.0)
        {
            const double dThetaDeg = dTheta * (180.0 / PC_PI);
            outVal = GainConstant * dThetaDeg;
        }
        else
        {
            double dThetaDeg = dTheta * (180.0 / PC_PI);
            dThetaDeg = m_phase->WrapDegreeSymmetric(dThetaDeg, MaxAngle);
            outVal = GainConstant * dThetaDeg;
        }
        break;
    }

    case PhaseComparator::Sinusoidal:
    {
        outVal = scaleRad2VoltDeg * std::sin(dTheta);
        break;
    }

    case PhaseComparator::Triangular:
    default:
    {
        const double triVal = m_phase->TriangularPhase(dTheta);
        outVal = scaleRad2VoltDeg * triVal;
        break;
    }
    }

    outputData[0] = std::complex<double>(outVal, 0.0);
    WriteOutputData(GetOutputPortName(0), outputData);
    m_phase->Advance();
    return true;
}

bool PhaseComparator_Block::TimeDrivenRun()
{
    BufferReader* s1 = GetInputPort("s1");
    BufferReader* s2 = GetInputPort("s2");
    Buffer* output = GetOutputPort("output");
    auto s1Data = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    auto s2Data = ReadInputData<EnvelopeSignal>(GetInputPortName(1));

    if(s1Data.empty() || s2Data.empty()) return true;
    m_s1Buffer.push_back(s1Data[0]);
    m_s2Buffer.push_back(s2Data[0]);

    std::vector<EnvelopeSignal> outputData(1);
    const double t = simulator_param.startTime + static_cast<double>(m_phase->GetCount()) / simulator_param.samplingRate;

    const double fc1 = s1->getCharacterizationFrequency();
    const double fc2 = s2->getCharacterizationFrequency();

    if (fc1 <= 0.0 || fc2 <= 0.0)
    {
        LOG_ERROR("PhaseComparator: inputs must be envelope signals with characterization frequency > 0.");
    }

    m_phase->fcOut_ = fc1;
    output->setCharacterizationFrequency(m_phase->fcOut_);

    const double fcOut = m_phase->fcOut_;

    if(m_s1Buffer.size() >= 1 && m_s2Buffer.size() >= 1) {
        const std::complex<double> x1 =
            m_s1Buffer[0].ConvertToNewFc(fc1, fcOut, t);
        const std::complex<double> x2 =
            m_s2Buffer[0].ConvertToNewFc(fc2, fcOut, t);

        const std::complex<double> z = x1 * std::conj(x2);
        const double dTheta = std::atan2(z.imag(), z.real());

        const double scaleRad2VoltDeg = GainConstant * (180.0 / PC_PI);

        double outVal = 0.0;

        switch (PhaseCharacteristicType)
        {
        case PhaseComparator::PhaseFreq:
        {
            if (MaxAngle <= 0.0)
            {
                const double dThetaDeg = dTheta * (180.0 / PC_PI);
                outVal = GainConstant * dThetaDeg;
            }
            else
            {
                double dThetaDeg = dTheta * (180.0 / PC_PI);
                dThetaDeg = m_phase->WrapDegreeSymmetric(dThetaDeg, MaxAngle);
                outVal = GainConstant * dThetaDeg;
            }
            break;
        }

        case PhaseComparator::Sinusoidal:
        {
            outVal = scaleRad2VoltDeg * std::sin(dTheta);
            break;
        }

        case PhaseComparator::Triangular:
        default:
        {
            const double triVal = m_phase->TriangularPhase(dTheta);
            outVal = scaleRad2VoltDeg * triVal;
            break;
        }
        }

        outputData[0] = std::complex<double>(outVal, 0.0);
        m_outputQueue.push(outputData[0]);
        m_phase->Advance();
    }
    //执行写入
    if (!m_outputQueue.empty()) {
        EnvelopeSignal outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[PhaseComparator_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << "," << outputValue.imag();
        m_s1Buffer.clear();
        m_s2Buffer.clear();
    }
    return true;
}

bool PhaseComparator_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool PhaseComparator_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_phase = std::make_unique<PhaseComparator>();
    SetDefaultParameters();
    try {
        GainConstant = std::stod(getParameter("GainConstant").Value);
        MaxAngle = std::stod(getParameter("MaxAngle").Value);
        PhaseCharacteristicType = ConvertStringToPhaseCharacteristicTypeEnum(getParameter("PhaseCharacteristicType").Value);
    } catch (...) {

    }
    SetParameters();

    AddInputPort("s1", m_phase->s1, 1, DataType::ENVELOPE_SIGNAL);
    AddInputPort("s2", m_phase->s2, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_phase->output, 1, DataType::ENVELOPE_SIGNAL);
    return true;
}

void PhaseComparator_Block::SetParameters()
{
    if(!m_phase) return;
    m_phase->GainConstant = GainConstant;
    m_phase->MaxAngle = MaxAngle;
    m_phase->PhaseCharacteristicType = PhaseCharacteristicType;
}

PhaseComparator::PhaseCharacteristicTypeEnum PhaseComparator_Block::ConvertStringToPhaseCharacteristicTypeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "phasefreq" || lower == "0") {
        return PhaseComparator::PhaseFreq;
    }
    if (lower == "sinusoidal" || lower == "1") {
        return PhaseComparator::Sinusoidal;
    }
    if (lower == "triangular" || lower == "1") {
        return PhaseComparator::Triangular;
    }
    return PhaseComparator::PhaseFreq;
}

void PhaseComparator_Block::SetDefaultParameters()
{
     simulator_param = getSimu();
     GainConstant = 1;
     MaxAngle = 360;
     PhaseCharacteristicType = PhaseComparator::PhaseFreq;
}
