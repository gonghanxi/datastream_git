#include "LogAmp_Block.h"

LogAmp_Block::LogAmp_Block(const std::string &name)
    :Block(name)
{

}

bool LogAmp_Block::Setup()
{
    Block::Setup();
    return true;
}

bool LogAmp_Block::Run()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(GetInputPortName(0));
    const double PI = std::acos(-1);

    double At = std::abs(inputData[0].complex());
    double PA = 10 * std::log10(0.5*At*At / m_RefR) + 30;
    double ep = m_Sensitivity * m_E*std::sin(2 * PI*(PA - m_PMin) / m_Ec);
    double VL = std::sqrt(2 * m_RefR*std::pow(10, (m_PMin - 30) / 10));
    double Mt = At > VL ? 20 * m_Sensitivity*std::log10(At / VL) + ep : 0;

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData(inputData.size());

    outputData[0] = Mt / At * inputData[0].complex();

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool LogAmp_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_LogAmp = std::make_unique<LogAmp>();

    SetDefaultParameters();

    try { m_Sensitivity = std::stod(getParameter("Sensitivity").Value); } catch (...) { }
    try { m_PMin = std::stod(getParameter("PMin").Value); } catch (...) { }
    try { m_E = std::stod(getParameter("E").Value); } catch (...) { }
    try { m_Ec = std::stod(getParameter("Ec").Value); } catch (...) { }
    try { m_RefR = std::stod(getParameter("RefR").Value); } catch (...) { }

    if(m_Ec == 0) {
        LOG_ERROR("Ec Must not equal 0");
        return false;
    }

    SetParameters();

    AddInputPort("input", m_LogAmp->input, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_LogAmp->output, 1, DataType::ENVELOPE_SIGNAL);
    return true;
}

void LogAmp_Block::SetParameters()
{
    if(!m_LogAmp) return;
    m_LogAmp->Sensitivity = m_Sensitivity;
    m_LogAmp->PMin = m_PMin;
    m_LogAmp->E = m_E;
    m_LogAmp->Ec = m_Ec;
    m_LogAmp->RefR = m_RefR;
}

void LogAmp_Block::SetDefaultParameters()
{
    m_Sensitivity = 0.1;
    m_PMin = -80;
    m_E = 0;
    m_Ec = 1;
    m_RefR = 50;
}
