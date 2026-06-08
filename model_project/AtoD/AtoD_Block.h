#ifndef ATOD_BLOCK_H
#define ATOD_BLOCK_H

#include "Block.h"
#include "AtoD.h"

#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AtoD_Block : public SystemVueModelBuilder::Block
{
public:
    AtoD_Block(const std::string& name);
    ~AtoD_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();
    void SetDefaultParamters();
    void SetParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    AtoD::OutputDigitalFormatEnum ConvertStringToOutputDigitalFormatEnum(const std::string& value);
    AtoD::DistortionModelEnum ConvertStringToDistortionModelEnum(const std::string& value);
    AtoD::EnableJitterEnum ConvertStringToEnableJitterEnum(const std::string& value);
    AtoD::PN_TypeEnum ConvertStringToPN_TypeEnum(const std::string& value);
    AtoD::FFT_SizeEnum ConvertStringToFFT_SizeEnum(const std::string& value);
    AtoD::SNR_ModelEnum ConvertStringToSNR_ModelEnum(const std::string& value);
    AtoD::ConversionTypeEnum ConvertStringToConversionTypeEnum(const std::string& value);
    AtoD::AntiAliasingFilterEnum ConvertStringToAntiAliasingFilterEnum(const std::string& value);

    std::unique_ptr<AtoD> m_atod;

    // --------- 参数 ---------
    int    m_NBits;
    double m_VRef;

    AtoD::OutputDigitalFormatEnum m_OutputDigitalFormat;
    AtoD::DistortionModelEnum     m_DistortionModel;

    AtoD::EnableJitterEnum m_EnableJitter;
    double m_RJrms;

    double* m_PhaseNoiseData;
    int     m_PhaseNoiseDataSize;
    AtoD::PN_TypeEnum m_PN_Type;

    double m_INL;
    double m_DNL;

    double m_ENOB;
    double m_SNR_dB;
    double m_H2_dBc;
    double m_H3_dBc;
    double m_H4_dBc;
    double m_H5_dBc;

    double m_SINAD_dB;
    double m_SFDR_dBc;
    AtoD::FFT_SizeEnum m_FFT_Size;

    AtoD::SNR_ModelEnum m_SNR_Model;
    double m_ThermalNoise_SNR_dBFS;
    double m_CenterFreq;
    double m_Level_dBFS;

    AtoD::ConversionTypeEnum m_ConversionType;
    double m_Clock;
    double m_Phase;

    int m_DownsampleFactor;
    int m_DownsamplePhase;
    AtoD::AntiAliasingFilterEnum m_AntiAliasingFilter;
    double m_ExcessBW;

    std::vector<double> primdata;
    double m_sampleRate;

    // ===== TimeDrivenRun 缓存与队列 =====
    std::vector<SystemVueModelBuilder::EnvelopeSignal> m_inputBuffer;
    std::queue<SystemVueModelBuilder::EnvelopeSignal> m_aoutQueue;
    std::queue<double> m_diQueue;
    std::queue<double> m_dqQueue;
    int m_inputRate = 1;

    SimuParameter simulator_param;
};

RegAlgo(AtoD_Block);
#endif // ATOD_BLOCK_H
