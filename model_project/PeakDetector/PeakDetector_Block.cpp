#include "PeakDetector_Block.h"
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

PeakDetector_Block::PeakDetector_Block(const std::string &name)
    :Block(name)
{

}

bool PeakDetector_Block::Setup()
{
    Block::Setup();
    bool bStatus = true;
    if (ChargeTimeConstant < 0)
    {
        LOG_ERROR("ChargeTimeConstant must be >= 0.");
        bStatus = false;
    }

    if (DecayTimeConstant < 0)
    {
        LOG_ERROR("DecayTimeConstant must be >= 0.");
        bStatus = false;
    }

    if (VTransWidth < 0)
    {
        LOG_ERROR("VTransWidth must be >= 0.");
        bStatus = false;
    }

    VOut = 0;

    return bStatus;
}

bool PeakDetector_Block::Run()
{
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    std::vector<EnvelopeSignal> outputData(1);
    SampleRate = getSimu().samplingRate;
    polaritySign = Polarity ? -1 : 1;
    VSignal = inputData[0].imag() ? polaritySign * std::abs(inputData[0].complex()) : inputData[0].real();

    // 输入信号检测处理阶段
    switch (Polarity)
    {
    case PeakDetector::positive:
        if (VThreshold < 0)
        {
            LOG_ERROR("Whren Polarity = positive, VThreshold must be >= 0.");
            return false;
        }

        if (VTransWidth == 0)
        {
            if (VSignal >= VThreshold)
            {
                VDetect = VSignal - VThreshold;
            }
            else
            {
                VDetect = 0;
            }
        }
        else
        {
            if (VSignal < VThreshold - VTransWidth / 2)
            {
                VDetect = 0;
            }
            else if (VSignal > VThreshold + VTransWidth / 2)
            {
                VDetect = VSignal - VThreshold;
            }
            else
            {
                VDetect = 0.5 / VTransWidth * std::pow((VSignal - (VThreshold - VTransWidth / 2)), 2);
            }
        }
        break;

    case PeakDetector::negative:
        if (VThreshold > 0)
        {
            LOG_ERROR("Whren Polarity = negative, VThreshold must be <= 0.");
            return false;
        }

        if (VTransWidth == 0)
        {
            if (VSignal < VThreshold)
            {
                VDetect = VSignal - VThreshold;
            }
            else
            {
                VDetect = 0;
            }
        }
        else
        {
            if (VSignal > VThreshold + VTransWidth / 2)
            {
                VDetect = 0;
            }
            else if (VSignal < VThreshold - VTransWidth / 2)
            {
                VDetect = VSignal - VThreshold;
            }
            else
            {
                VDetect = 0.5 / VTransWidth * std::pow((VSignal - (VThreshold + VTransWidth / 2)), 2);
            }
        }
        break;

    default:
        break;
    }

    // 信号输出处理阶段
    if (ChargeTimeConstant == 0 && VDetect > VOut)
    {
        VOut = VDetect;
    }

    else if (ChargeTimeConstant > 0 && VDetect > VOut)
    {
        VTest = VOut + (VDetect - VOut)*(1.0 - std::exp(-1 / ChargeTimeConstant / SampleRate));
        if (VTest > VOut)
        {
            VOut = VTest;
        }
    }

    if (DecayTimeConstant == 0 && VDetect < VOut)
    {
        VOut = VDetect;
    }

    else if (DecayTimeConstant > 0 && VDetect < VOut)
    {
        VTest = VOut * std::exp(-1 / DecayTimeConstant / SampleRate);
        if (VTest < VOut)
        {
            VOut = VTest;
        }
    }

    outputData[0] = VOut;
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool PeakDetector_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_peak = std::make_unique<PeakDetector>();
    SetDefaultParameters();
    try {
        ChargeTimeConstant = std::stod(getParameter("ChargeTimeConstant").Value);
        DecayTimeConstant = std::stod(getParameter("DecayTimeConstant").Value);
        VThreshold = std::stod(getParameter("VThreshold").Value);
        VTransWidth = std::stod(getParameter("VTransWidth").Value);
        Polarity = ConvertStringToSelectedPolarity(getParameter("Polarity").Value);
    } catch (...) {

    }
    SetParameters();

    AddInputPort("input", m_peak->input, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_peak->output, 1, DataType::ENVELOPE_SIGNAL);
    return true;
}

void PeakDetector_Block::SetParameters()
{
    if(!m_peak) return;
    m_peak->ChargeTimeConstant = ChargeTimeConstant;
    m_peak->DecayTimeConstant = DecayTimeConstant;
    m_peak->VThreshold = VThreshold;
    m_peak->VTransWidth = VTransWidth;
    m_peak->Polarity = Polarity;
}

PeakDetector::SelectedPolarity PeakDetector_Block::ConvertStringToSelectedPolarity(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "positive" || lower == "0") {
        return PeakDetector::positive;
    }
    if (lower == "negative" || lower == "1") {
        return PeakDetector::negative;
    }
    return PeakDetector::positive;
}

void PeakDetector_Block::SetDefaultParameters()
{
     ChargeTimeConstant = 0;
     DecayTimeConstant = 20e-6;
     VThreshold = 0;
     VTransWidth = 0;
     Polarity = PeakDetector::positive;
}
