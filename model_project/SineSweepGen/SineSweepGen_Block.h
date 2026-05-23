#pragma once

#include "SineSweepGen.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SineSweepGen_Block : public SystemVueModelBuilder::Block
{
public:
    SineSweepGen_Block(const std::string& name);
    ~SineSweepGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    SineSweepGen::SelectedFSweepType ConvertStringToFSweepType(const std::string& value);
    SineSweepGen::SelectedShowAdvancedParams ConvertStringToShowAdvanced(const std::string& value);
    SineSweepGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<SineSweepGen> m_sineSweepGen;

    double m_amplitude;
    double m_offset;
    SineSweepGen::SelectedFSweepType m_fSweepType;
    double m_startFreq;
    double m_stopFreq;
    double m_phase;
    double m_sweepPeriod;
    SineSweepGen::SelectedShowAdvancedParams m_showAdvancedParams;
    SineSweepGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;

    SimuParameter simulator_param;
};

RegAlgo(SineSweepGen_Block);
