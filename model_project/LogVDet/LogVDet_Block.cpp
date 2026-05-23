#include "LogVDet_Block.h"

LogVDet_Block::LogVDet_Block(const std::string &name)
    :Block(name)
{

}

bool LogVDet_Block::Setup()
{
    Block::Setup();
    return true;
}

bool LogVDet_Block::Run()
{
    auto inputData = ReadInputData<EnvelopeSignal>("input");
    std::vector<EnvelopeSignal> outputData;
    const double PI = std::acos(-1);

    double At = std::abs(inputData[0].complex());
    double PA = 10 * std::log10(0.5*At*At / RefR) + 30;
    double ep = Sensitivity * E*std::sin(2 * PI*(PA - PMin) / Ec);
    double VL = std::sqrt(2 * RefR*std::pow(10, (PMin - 30) / 10));

    outputData[0] = At > VL ? 20 * Sensitivity*std::log10(At / VL) + ep : 0;
    WriteOutputData("output", outputData);
    return true;
}

bool LogVDet_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_log = std::make_unique<LogVDet>();
    SetDefaultParameters();
    try {
        Sensitivity = std::stod(getParameter("Sensitivity").Value);
        PMin = std::stod(getParameter("PMin").Value);
        E = std::stod(getParameter("E").Value);
        Ec = std::stod(getParameter("Ec").Value);
        RefR = std::stod(getParameter("RefR").Value);
    } catch (...) {

    }
    SetParameters();
    AddInputPort("input", m_log->input, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_log->output, 1, DataType::ENVELOPE_SIGNAL);
    return true;
}

void LogVDet_Block::SetParameters()
{
    if(!m_log) return;
    m_log->Sensitivity = Sensitivity;
    m_log->PMin = PMin;
    m_log->E = E;
    m_log->Ec = Ec;
    m_log->RefR = RefR;
}

void LogVDet_Block::SetDefaultParameters()
{
    Sensitivity = 0.1;
    PMin = -80;
    E = 0;
    Ec = 0;
    RefR = 50;
}
