#ifndef RADAR_CW_BLOCK_H
#define RADAR_CW_BLOCK_H

#include "Block.h"
#include "RADAR_CW.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_CW_Block : public SystemVueModelBuilder::Block {
public:

    RADAR_CW_Block(const std::string& name);
    ~RADAR_CW_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double sampleRate = 1e6,
                            double amplitude = 1.0,
                            double lowerFreq = 10e3,
                            double deltaFreq = 50e3,
                            double period = 1e-4,
                            double freqDowntime = 1e-5,
                            double freqUptime = 1e-5,
                            double freqFixtime = 1e-5,
                            double offTime = 1e-5,
                            RADAR_CW::Waveform_typeEnum waveformType = RADAR_CW::Sawtooth);

    int GetGeneratedSampleCount() const;
private:
    RADAR_CW::Waveform_typeEnum ConvertStringToWaveformEnum(const std::string& value);

    void SetDefaultParameters();
    std::unique_ptr<RADAR_CW> m_radarCW;
    double m_sampleRate;
    double m_amplitude;
    double m_lowerFreq;
    double m_deltaFreq;
    double m_period;
    double m_FreqDownTime;
    double m_FreqUpTime;
    double m_FreqFixTime;
    double m_OffTime;
    RADAR_CW::Waveform_typeEnum m_waveformtype;
    int m_counter;

};

RegAlgo(RADAR_CW_Block);


#endif // RADAR_CW_BLOCK_H
