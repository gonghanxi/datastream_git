#pragma once

#include "RampSweepGen.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RampSweepGen_Block : public SystemVueModelBuilder::Block
{
public:
    RampSweepGen_Block(const std::string& name);
    ~RampSweepGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RampSweepGen::SelectedFSweepType ConvertStringToFSweepType(const std::string& value);
    RampSweepGen::SelectedPolarity ConvertStringToPolarity(const std::string& value);
    RampSweepGen::SelectedShowAdvancedParams ConvertStringToShowAdvanced(const std::string& value);
    RampSweepGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<RampSweepGen> m_rampSweepGen;

    double m_loLevel;
    double m_hiLevel;
    RampSweepGen::SelectedFSweepType m_fSweepType;
    double m_startFreq;
    double m_stopFreq;
    double m_phase;
    double m_sweepPeriod;
    double m_symmetry;
    RampSweepGen::SelectedPolarity m_polarity;
    RampSweepGen::SelectedShowAdvancedParams m_showAdvancedParams;
    RampSweepGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;

    SimuParameter simulator_param;
};

RegAlgo(RampSweepGen_Block);
