#pragma once

#include "ChirpGen.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ChirpGen_Block : public SystemVueModelBuilder::Block
{
public:
    ChirpGen_Block(const std::string& name);
    ~ChirpGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    ChirpGen::ShowAdvancedParamsEnum ConvertStringToShowAdvanced(const std::string& value);
    ChirpGen::SampleRateOptionEnum ConvertStringToSampleRateOption(const std::string& value);

    void SetDefaultParamters();

    std::unique_ptr<ChirpGen> m_chirpGen;

    double m_amplitude;
    double m_offset;
    double m_startFreq;
    double m_stopFreq;
    double m_phase;
    double m_sweepPeriod;
    ChirpGen::ShowAdvancedParamsEnum m_showAdvancedParams;
    ChirpGen::SampleRateOptionEnum m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;

    int m_counter = 0;
    SimuParameter simulator_param;
};

RegAlgo(ChirpGen_Block);
