#include "Hysteresis_Block.h"

Hysteresis_Block::Hysteresis_Block(const std::string &name)
    :Block(name)
{

}

bool Hysteresis_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Hysteresis_Block::Run()
{
    bool bStatus = true;

    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(1);
    outputData.reserve(1);
    SampleRate = getSimu().samplingRate;

    if (Bandwidth < 0 || Bandwidth > SampleRate)
    {
        LOG_ERROR("Bandwidth should be: 0 <= Bandwidth <= SampleRate");
        bStatus = false;
    }

    Difference = (inputData[0] - InternalState);

    if (std::abs(Difference) > Backlash)
    {
        InternalState += (Difference - Backlash * Difference / std::abs(Difference)) * Bandwidth / SampleRate;
    }

    outputData[0] = InternalState * Gain;
    WriteOutputData(GetOutputPortName(0), outputData);
    return bStatus;
}

bool Hysteresis_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Hysteresis = std::make_unique<Hysteresis>();
    SetDefaultParameters();
    try { Bandwidth = std::stod(getParameter("Bandwidth").Value); } catch (...) {}
    try { Backlash = std::stod(getParameter("Backlash").Value); } catch (...) {}
    try { Gain = std::stod(getParameter("Gain").Value); } catch (...) {}
    SetParameters();
    if(!m_Hysteresis->Setup()) return false;
    AddInputPort("input", m_Hysteresis->input, 1, DataType::TIMED_DOUBLE);
    AddOutputPort("output", m_Hysteresis->output, 1, DataType::TIMED_DOUBLE);
    return true;
}

void Hysteresis_Block::SetParameters()
{
    if(!m_Hysteresis) return;
    m_Hysteresis->Bandwidth = Bandwidth;
    m_Hysteresis->Backlash = Backlash;
    m_Hysteresis->Gain = Gain;
}

void Hysteresis_Block::SetDefaultParameters()
{
    Bandwidth = 0;
    Backlash = 0;
    Gain = 1;
}
