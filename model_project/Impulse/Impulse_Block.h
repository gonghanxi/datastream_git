#pragma once

#include "Impulse.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Impulse_Block : public SystemVueModelBuilder::Block
{
public:
    Impulse_Block(const std::string& name);
    ~Impulse_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    Impulse::SelectedNoOrYes ConvertStringToNoOrYes(const std::string& value);
    Impulse::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    Impulse::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<Impulse> m_impulse;

    double m_level;
    Impulse::SelectedNoOrYes m_scaleBySampleRate;
    Impulse::SelectedNoOrYes m_showAdvancedParams;
    Impulse::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;
    Impulse::SelectedBurstMode m_burstMode;
    double m_burstLength;
    double m_burstPeriod;
    double m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(Impulse_Block);
