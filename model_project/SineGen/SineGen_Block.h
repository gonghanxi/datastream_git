#ifndef SINEGEN_BLOCK_H
#define SINEGEN_BLOCK_H

#include "Block.h"
#include "SineGen.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SineGen_Block : public SystemVueModelBuilder::Block
{
public:
    SineGen_Block(const std::string& name);
    ~SineGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    SineGen::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    SineGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    SineGen::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    std::unique_ptr<SineGen> m_sineGen;

    double m_amplitude;
    double m_offset;
    double m_frequency;
    double m_phase;
    SineGen::SelectedShowAdvancedParams m_showAdvancedParams;
    SineGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;
    SineGen::SelectedBurstMode m_burstMode;
    double m_burstLength;
    double m_burstPeriod;
    double m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(SineGen_Block);

#endif // SINEGEN_BLOCK_H
