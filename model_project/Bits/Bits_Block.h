#pragma once

#include "Bits.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Bits_Block : public SystemVueModelBuilder::Block
{
public:
    Bits_Block(const std::string& name);
    ~Bits_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    Bits::ShowAdvancedParamsEnum ConvertStringToShowAdvanced(const std::string& value);
    Bits::SampleRateOptionEnum ConvertStringToSampleRateOption(const std::string& value);
    Bits::BurstModeEnum ConvertStringToBurstMode(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<Bits> m_bits;

    double m_probOfZero;
    double m_bitRate;
    Bits::ShowAdvancedParamsEnum m_showAdvancedParams;
    Bits::SampleRateOptionEnum m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;
    Bits::BurstModeEnum m_burstMode;
    int m_burstLength;
    int m_burstPeriod;
    int m_burstDelay;

    bool m_previousBitValue = false;
    SimuParameter simulator_param;
};

RegAlgo(Bits_Block);
