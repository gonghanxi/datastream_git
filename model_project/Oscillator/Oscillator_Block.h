#pragma once

#include "Oscillator.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Oscillator_Block : public SystemVueModelBuilder::Block
{
public:
    Oscillator_Block(const std::string& name);
    ~Oscillator_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    Oscillator::SelectedYesOrNo ConvertStringToYesOrNo(const std::string& value);
    Oscillator::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    std::unique_ptr<Oscillator> m_oscillator;

    double m_frequency = 1e6;
    double m_power = 0.010;
    double m_phase = 0.0;
    Oscillator::SelectedYesOrNo m_randomPhase = Oscillator::No;
    double m_ndensity = 0.0;
    double m_refR = 50.0;
    Oscillator::SelectedYesOrNo m_showAdvancedParams = Oscillator::No;
    Oscillator::SelectedSampleRateOption m_sampleRateOption = Oscillator::TimedFromSchematic;
    double m_sampleRate = 0.0;
    double m_initialDelay = 0.0;

    SimuParameter simulator_param;
};

RegAlgo(Oscillator_Block);
