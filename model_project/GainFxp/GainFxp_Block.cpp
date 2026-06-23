#include "GainFxp_Block.h"
#include <cmath>

GainFxp_Block::GainFxp_Block(const std::string &name)
    :Block(name), m_gain(1.0), m_fxpPos(4)
{
}

void GainFxp_Block::SetParameters(double gain)
{
    m_gain = gain;
    if(m_GainFxp) {
        m_GainFxp->Gain = gain;
    }
}

bool GainFxp_Block::Setup()
{
    Block::Setup();
    return true;
}

bool GainFxp_Block::Run()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPortName);
    if(inputData.empty()) {
        if(IsVariableStepMode()) {
            return true;
        }
        return false;
    }
    qDebug() << "GainFxp_Block::Run - inputData: " << inputData.size();

    double factor = std::pow(10.0, m_fxpPos);

    std::vector<double> outputData;
    outputData.reserve(inputData.size());
    for(size_t i = 0; i < inputData.size(); i++) {
        double outputSample = m_gain * inputData[i];
        outputSample = std::trunc(outputSample * factor) / factor;
        outputData.push_back(outputSample);
    }

    WriteOutputData(outputPortName, outputData);
    return true;
}

bool GainFxp_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_GainFxp = std::make_unique<GainFxp>();

    AddInputPort("input", m_GainFxp->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_GainFxp->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParameters();
    try { m_gain = std::stod(getParameter("Gain").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Gain', using default value."); }
    try { m_fxpPos = std::stoi(getParameter("FxpPos").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FxpPos', using default value."); }
    SetParameters(m_gain);
    return true;
}

void GainFxp_Block::SetDefaultParameters()
{
    m_gain = 1.0;
    m_fxpPos = 4;
}
