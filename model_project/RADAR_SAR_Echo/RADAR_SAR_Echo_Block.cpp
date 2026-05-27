#include "RADAR_SAR_Echo_Block.h"
#include <algorithm>
#include <cctype>
#include <vector>

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

RADAR_SAR_Echo_Block::RADAR_SAR_Echo_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_SAR_Echo_Block::SetDefaultParamters()
{
    m_SAR_Mode = RADAR_SAR_Echo::Stripmap;
    m_Fc = 1e9;
    m_Xmin = 0;
    m_Xmax = 50;
    m_Yc = 10000;
    m_Y0 = 500;
    m_H = 5000;
    m_Vr = 100;
    m_D = 4;
    m_Tr = 5e-6;
    m_Br = 30e6;
    m_SampleRate = 100e6;
    m_EchoGenerate_Mode = RADAR_SAR_Echo::Point_Target;
    m_TargetInfo.Resize(1, 3);
    m_TargetInfo(0,0) = 0;
    m_TargetInfo(0,1) = 0;
    m_TargetInfo(0,2) = 1;
}

void RADAR_SAR_Echo_Block::SetParameters()
{
    if (!m_RADAR_SAR_Echo) {
        return;
    }

    m_RADAR_SAR_Echo->SAR_Mode = m_SAR_Mode;
    m_RADAR_SAR_Echo->Fc = m_Fc;
    m_RADAR_SAR_Echo->Xmin = m_Xmin;
    m_RADAR_SAR_Echo->Xmax = m_Xmax;
    m_RADAR_SAR_Echo->Yc = m_Yc;
    m_RADAR_SAR_Echo->Y0 = m_Y0;
    m_RADAR_SAR_Echo->H = m_H;
    m_RADAR_SAR_Echo->Vr = m_Vr;
    m_RADAR_SAR_Echo->D = m_D;
    m_RADAR_SAR_Echo->Tr = m_Tr;
    m_RADAR_SAR_Echo->Br = m_Br;
    m_RADAR_SAR_Echo->SampleRate = m_SampleRate;
    m_RADAR_SAR_Echo->EchoGenerate_Mode = m_EchoGenerate_Mode;
    m_RADAR_SAR_Echo->TargetInfo = m_TargetInfo;
}

bool RADAR_SAR_Echo_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_SAR_Echo_Block::Run()
{
    if (!m_RADAR_SAR_Echo->Run()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;

    int totalSize = m_RADAR_SAR_Echo->m_Nslow * m_RADAR_SAR_Echo->m_Nfast;

    qDebug()<<"RADAR_SAR_Echo: totalSize"<<totalSize;

    for (int i = 0; i< totalSize; i++) {
        outputData.push_back(std::complex<double>(
         m_RADAR_SAR_Echo->outputData[i].real(),
         m_RADAR_SAR_Echo->outputData[i].imag()
         ));
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool RADAR_SAR_Echo_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_RADAR_SAR_Echo = std::make_unique<RADAR_SAR_Echo>();

    SetDefaultParamters();
    //simulator_param = getSimu();

    try{ m_SAR_Mode = ConvertStringToSAR_Mode(getParameter("SAR_Mode").Value); } catch(...) {}
    try{ m_Fc = std::stod(getParameter("Fc").Value); } catch (...) { }
    try{ m_Xmin = std::stod(getParameter("Xmin").Value);} catch (...) { }
    try{ m_Xmax = std::stod(getParameter("Xmax").Value);} catch(...) { }
    try{ m_Yc = std::stod(getParameter("Yc").Value);} catch(...) { }
    try{ m_Y0 = std::stod(getParameter("Y0").Value);} catch(...) { }
    try{ m_H = std::stod(getParameter("H").Value);} catch(...) { }
    try{ m_Vr = std::stod(getParameter("Vr").Value);} catch(...) { }
    try{ m_D = std::stod(getParameter("D").Value);} catch(...) { }
    try{ m_Tr = std::stod(getParameter("Tr").Value);} catch(...) { }
    try{ m_Br = std::stod(getParameter("Br").Value);} catch(...) { }
    try{ m_SampleRate = std::stod(getParameter("SampleRate").Value);} catch(...) { }
    try{ m_EchoGenerate_Mode = ConvertStringToEchoGenerate_Mode(getParameter("EchoGenerate_Mode").Value);} catch(...) {}
    try{ m_TargetInfo = ParseStringToMatrix<double>(getParameter("TargetInfo").Value);} catch(...) { }

    SetParameters();

    if(!m_RADAR_SAR_Echo->Setup()) {
        return false;
    }

    int rate = m_RADAR_SAR_Echo->output.GetRate();


    AddOutputPort("output", m_RADAR_SAR_Echo->output, rate, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;

}

RADAR_SAR_Echo::SelectedSAR_Mode RADAR_SAR_Echo_Block::ConvertStringToSAR_Mode(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "stripmap") {
        return RADAR_SAR_Echo::Stripmap;
    }
    return RADAR_SAR_Echo::Stripmap;
}

RADAR_SAR_Echo::SelectedEchoGenerate_Mode RADAR_SAR_Echo_Block::ConvertStringToEchoGenerate_Mode(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "point_target") {
        return RADAR_SAR_Echo::Point_Target;
    }
    return RADAR_SAR_Echo::Point_Target;
}
