#pragma once

#include "SquareSweepGen.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SquareSweepGen_Block : public SystemVueModelBuilder::Block
{
public:
    SquareSweepGen_Block(const std::string& name);
    ~SquareSweepGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    SquareSweepGen::SelectedFSweepType ConvertStringToFSweepType(const std::string& value);
    SquareSweepGen::SelectedPolarity ConvertStringToPolarity(const std::string& value);
    SquareSweepGen::SelectedShowAdvancedParams ConvertStringToShowAdvanced(const std::string& value);
    SquareSweepGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<SquareSweepGen> m_squareSweepGen;

    double m_loLevel;
    double m_hiLevel;
    SquareSweepGen::SelectedFSweepType m_fSweepType;
    double m_startFreq;
    double m_stopFreq;
    double m_phase;
    double m_sweepPeriod;
    double m_dutyCycle;
    SquareSweepGen::SelectedPolarity m_polarity;
    SquareSweepGen::SelectedShowAdvancedParams m_showAdvancedParams;
    SquareSweepGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;

    SimuParameter simulator_param;
};

RegAlgo(SquareSweepGen_Block);
