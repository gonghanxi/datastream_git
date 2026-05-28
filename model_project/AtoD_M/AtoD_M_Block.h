#ifndef ATOD_M_BLOCK_H
#define ATOD_M_BLOCK_H

#include "Block.h"
#include "AtoD_M.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AtoD_M_Block : public SystemVueModelBuilder::Block
{
public:
    AtoD_M_Block(const std::string& name);
    ~AtoD_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    AtoD_M::OutputDigitalFormatEnum ConvertStringToOutputDigitalFormatEnum(const std::string& value);
    AtoD_M::DistortionModelEnum ConvertStringToDistortionModelEnum(const std::string& value);
    AtoD_M::EnableJitterEnum ConvertStringToEnableJitterEnum(const std::string& value);
    AtoD_M::PN_TypeEnum ConvertStringToPN_TypeEnum(const std::string& value);
    AtoD_M::FFT_SizeEnum ConvertStringToFFT_SizeEnum(const std::string& value);
    AtoD_M::SNR_ModelEnum ConvertStringToSNR_ModelEnum(const std::string& value);
    AtoD_M::ConversionTypeEnum ConvertStringToConversionTypeEnum(const std::string& value);
    AtoD_M::AntiAliasingFilterEnum ConvertStringToAntiAliasingFilterEnum(const std::string& value);

    std::unique_ptr<AtoD_M> m_atod_m;

    // --------- 参数 ---------
    int    m_NBits;
    double m_VRef;

    AtoD_M::OutputDigitalFormatEnum m_OutputDigitalFormat;
    AtoD_M::DistortionModelEnum     m_DistortionModel;

    AtoD_M::EnableJitterEnum m_EnableJitter;
    double m_RJrms;
    
    double* m_PhaseNoiseData;
    int     m_PhaseNoiseDataSize;
    AtoD_M::PN_TypeEnum m_PN_Type;

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
    AtoD_M::FFT_SizeEnum m_FFT_Size;

    AtoD_M::SNR_ModelEnum m_SNR_Model;
    double m_ThermalNoise_SNR_dBFS;
    double m_CenterFreq;
    double m_Level_dBFS;

    AtoD_M::ConversionTypeEnum m_ConversionType;
    double m_Clock;
    double m_Phase;

    int m_DownsampleFactor;
    int m_DownsamplePhase;
    AtoD_M::AntiAliasingFilterEnum m_AntiAliasingFilter;
    double m_ExcessBW;

    std::vector<double> primdata;
    double m_sampleRate;
    SimuParameter simulator_param;
};

RegAlgo(AtoD_M_Block);
#endif // ATOD_M_BLOCK_H
