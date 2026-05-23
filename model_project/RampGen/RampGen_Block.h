#ifndef RAMPGEN_BLOCK_H
#define RAMPGEN_BLOCK_H

#include "Block.h"
#include "RampGen.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RampGen_Block : public SystemVueModelBuilder::Block
{
public:
    RampGen_Block(const std::string& name);
    ~RampGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    RampGen::SelectedPolarity ConvertStringToPolarity(const std::string& value);
    RampGen::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    RampGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    RampGen::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    std::unique_ptr<RampGen> m_rampGen;

    double m_loLevel;
    double m_hiLevel;
    double m_frequency;
    double m_phase;
    double m_symmetry;
    RampGen::SelectedPolarity m_polarity;
    RampGen::SelectedShowAdvancedParams m_showAdvancedParams;
    RampGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;
    RampGen::SelectedBurstMode m_burstMode;
    double m_burstLength;
    double m_burstPeriod;
    double m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(RampGen_Block);

#endif // RAMPGEN_BLOCK_H
