#include "PulseGen_Block.h"

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

PulseGen_Block::PulseGen_Block(const std::string& name)
    :Block(name)
{

}

bool PulseGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool PulseGen_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_PulseGen->Run()) {
        return false;
    }

    std::vector<double> Data;
    Data.push_back(m_PulseGen->output[0]);
    WriteOutputData(GetOutputPortName(0), Data);

    m_PulseGen->Advance();
    return true;
}

bool PulseGen_Block::Initialize()
{

    SetBlockType(Block::BlockType::SOURCE);

    m_PulseGen = std::make_unique<PulseGen>();

    SetDefaultParameters();

    // 从参数系统获取参数值
    try { m_LoLevel = std::stod(getParameter("LoLevel").Value); } catch (...) { }
    try { m_HiLevel = std::stod(getParameter("HiLevel").Value); } catch (...) { }
    try { m_Period = std::stod(getParameter("Period").Value); } catch (...) { }
    try { m_Phase = std::stod(getParameter("Phase").Value); } catch (...) { }
    try { m_PulseWidth = std::stod(getParameter("PulseWidth").Value); } catch (...) { }
    try { m_EdgeTime = std::stod(getParameter("EdgeTime").Value); } catch (...) { }
    try { m_RisingEdgeTime = std::stod(getParameter("RisingEdgeTime").Value); } catch (...) { }
    try { m_FallingEdgeTime = std::stod(getParameter("FallingEdgeTime").Value); } catch (...) { }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    try { m_EdgeSymmetry = ConvertStringToEdgeSymmetrys(getParameter("EdgeSymmetry").Value); } catch (...) { }
    try { m_Polarity = ConvertStringToPolaritys(getParameter("Polarity").Value); } catch (...) { }

    SetParameters();

    if (m_PulseWidth < m_EdgeTime || m_PulseWidth > m_Period - m_EdgeTime) {
        qDebug() << "PulseGen_Block::Initialize - m_PulseWidth: " << m_PulseWidth;
        qDebug() << "PulseGen_Block::Initialize - m_EdgeTime: " << m_EdgeTime;
        qDebug() << "PulseGen_Block::Initialize - m_Period - m_EdgeTime: " << m_Period - m_EdgeTime;

        qDebug() << "PulseGen_Block::Initialize - error";
    }
    else {
        qDebug() << "PulseGen_Block::Initialize - no error";
    }

    if(!m_PulseGen->Setup()) {
        return false;
    }

    // 添加输入输出端口
    AddOutputPort("output", m_PulseGen->output, 1, Block::DataType::TIMED_DOUBLE);

    return true;
}

void PulseGen_Block::SetParameters()
{
     if(!m_PulseGen) {
         return;
     }
     m_PulseGen->LoLevel = m_LoLevel;
     m_PulseGen->HiLevel = m_HiLevel;
     m_PulseGen->Period = m_Period;
     m_PulseGen->Phase = m_Phase;
     m_PulseGen->PulseWidth = m_PulseWidth;
     m_PulseGen->EdgeSymmetry = m_EdgeSymmetry;
     m_PulseGen->EdgeTime = m_EdgeTime;
     m_PulseGen->RisingEdgeTime = m_RisingEdgeTime;
     m_PulseGen->FallingEdgeTime = m_FallingEdgeTime;
     m_PulseGen->Polarity = m_Polarity;
     m_PulseGen->SampleRate = m_SampleRate;
}

PulseGen::EdgeSymmetrys PulseGen_Block::ConvertStringToEdgeSymmetrys(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "symmetric" || lower == "0") {
        return PulseGen::Symmetric;
    }
    if (lower == "asymmetric" || lower == "1") {
        return PulseGen::Asymmetric;
    }
    return PulseGen::Symmetric;
}

PulseGen::Polaritys PulseGen_Block::ConvertStringToPolaritys(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "normal" || lower == "0") {
        return PulseGen::normal;
    }
    if (lower == "inverted" || lower == "1") {
        return PulseGen::inverted;
    }
    return PulseGen::normal;
}

void PulseGen_Block::SetDefaultParameters()
{
    m_LoLevel = 0;
    m_HiLevel = 1;
    m_Period = 200e-6;
    m_Phase = 0;
    m_PulseWidth = 100e-6;
    m_EdgeSymmetry = PulseGen::Symmetric;
    m_EdgeTime = 50e-6;
    m_RisingEdgeTime = 50e-6;
    m_FallingEdgeTime = 50e-6;
    m_Polarity = PulseGen::normal;
    m_SampleRate = 10e6;
}

