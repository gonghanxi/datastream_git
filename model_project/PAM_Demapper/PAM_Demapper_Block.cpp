#include "PAM_Demapper_Block.h"
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
PAM_Demapper_Block::PAM_Demapper_Block(const std::string &name)
    :Block(name)
{

}

bool PAM_Demapper_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    if (NumBits < 1)
    {
        LOG_ERROR("PAM_Demapper: NumBits must be >= 1");
        NumBits = 1;
    }
    m_pam->update_cache();
    return true;
}

bool PAM_Demapper_Block::DataStreamRun()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<int> BitsData(NumBits);
    std::vector<double> AmplitudeData(1);

    const double x = inputData[0];

    int idx = 0;

    if (m_pam->m_levels <= 1 || m_pam->m_step == 0.0)
    {
        idx = 0;
    }
    else
    {
        const double t = (x - LowLevel) / m_pam->m_step;
        int qi = (int)std::floor(t + 0.5);
        if (qi < 0) qi = 0;
        if (qi > (m_pam->m_levels - 1)) qi = (m_pam->m_levels - 1);
        idx = qi;
    }

    double level = LowLevel;
    if (m_pam->m_levels > 1)
        level = LowLevel + double(idx) * m_pam->m_step;

    AmplitudeData[0] = level;

    if (BitOrder == PAM_Demapper::MSBFirst)
    {
        for (int b = 0; b < NumBits; ++b)
        {
            const int shift = (NumBits - 1 - b);
            BitsData[b] = (idx >> shift) & 1;
        }
    }
    else
    {
        for (int b = 0; b < NumBits; ++b)
        {
            BitsData[b] = (idx >> b) & 1;
        }
    }
    WriteOutputData(GetOutputPortName(0), BitsData);
    WriteOutputData(GetOutputPortName(1), AmplitudeData);
    return true;
}

bool PAM_Demapper_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if(inputData.empty()) return true;

    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(NumBits)) {
        std::vector<int> BitsData(NumBits);
        std::vector<double> AmplitudeData(1);

        const double x = m_inputBuffer[0];

        int idx = 0;

        if (m_pam->m_levels <= 1 || m_pam->m_step == 0.0)
        {
            idx = 0;
        }
        else
        {
            const double t = (x - LowLevel) / m_pam->m_step;
            int qi = (int)std::floor(t + 0.5);
            if (qi < 0) qi = 0;
            if (qi > (m_pam->m_levels - 1)) qi = (m_pam->m_levels - 1);
            idx = qi;
        }

        double level = LowLevel;
        if (m_pam->m_levels > 1)
            level = LowLevel + double(idx) * m_pam->m_step;

        AmplitudeData[0] = level;

        if (BitOrder == PAM_Demapper::MSBFirst)
        {
            for (int b = 0; b < NumBits; ++b)
            {
                const int shift = (NumBits - 1 - b);
                BitsData[b] = (idx >> shift) & 1;
            }
        }
        else
        {
            for (int b = 0; b < NumBits; ++b)
            {
                BitsData[b] = (idx >> b) & 1;
            }
        }

        for(const auto& val : BitsData) m_outputQueue.push(val);
        //执行写入
        if (!m_outputQueue.empty()) {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
            WriteOutputData(GetOutputPortName(1), AmplitudeData);
            m_lastOutput = outputValue;

            qDebug() << "[PAM_Demapper_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue << "|" << AmplitudeData[0];
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

bool PAM_Demapper_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool PAM_Demapper_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_pam = std::make_unique<PAM_Demapper>();
    SetDefaultParameters();
    try { NumBits = std::stoi(getParameter("NumBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumBits', using default value."); }
    try { BitOrder = ConvertStringToBitOrderE(getParameter("BitOrder").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BitOrder', using default value."); }
    try { HighLevel = std::stod(getParameter("HighLevel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'HighLevel', using default value."); }
    try { LowLevel = std::stod(getParameter("LowLevel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'LowLevel', using default value."); }
    SetParameters();
    m_pam->update_cache();

    m_pam->input.SetRate(1);
    m_pam->Bits.SetRate(NumBits);
    m_pam->Amplitude.SetRate(1);

    AddInputPort("input", m_pam->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Bits", m_pam->Bits, static_cast<size_t>(NumBits), DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("Amplitude", m_pam->Amplitude, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void PAM_Demapper_Block::SetParameters()
{
    if(!m_pam) return;
    m_pam->NumBits = NumBits;
    m_pam->BitOrder = BitOrder;
    m_pam->HighLevel = HighLevel;
    m_pam->LowLevel = LowLevel;
}

PAM_Demapper::BitOrderE PAM_Demapper_Block::ConvertStringToBitOrderE(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "lsbfirst" || lower == "0") return PAM_Demapper::LSBFirst;
    if(lower == "msbfirst" || lower == "1") return PAM_Demapper::MSBFirst;
    return PAM_Demapper::MSBFirst;
}

void PAM_Demapper_Block::SetDefaultParameters()
{
    NumBits = 4;
    BitOrder = PAM_Demapper::MSBFirst;
    HighLevel = 1;
    LowLevel = -1;
}


