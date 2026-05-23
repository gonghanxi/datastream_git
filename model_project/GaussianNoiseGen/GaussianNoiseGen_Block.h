#pragma once
#include "GaussianNoiseGen.h"
#include "Block.h"
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API GaussianNoiseGen_Block : public SystemVueModelBuilder::Block
{
public:
    GaussianNoiseGen_Block(const std::string& name);
    ~GaussianNoiseGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    GaussianNoiseGen::SelectedShowAdvancedParams ConvertStringToShowAdvanced(const std::string& value);
    GaussianNoiseGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    GaussianNoiseGen::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<GaussianNoiseGen> m_gaussian;

    double m_nDensity;
    double m_refR;
    GaussianNoiseGen::SelectedShowAdvancedParams m_showAdvancedParams;
    GaussianNoiseGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;
    GaussianNoiseGen::SelectedBurstMode m_burstMode;
    int m_burstLength;
    int m_burstPeriod;
    int m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(GaussianNoiseGen_Block);
