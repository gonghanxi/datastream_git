#ifndef RADAR_RX_BLOCK_H
#define RADAR_RX_BLOCK_H

#include "RADAR_Rx.h"
#include "Block.h"
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Rx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Rx_Block(const std::string& name);
    ~RADAR_Rx_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool ModelSetup();
    void UpdateCharacterizationFrequency();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_Rx> m_rx;

    // ========== 基本参数 ==========
    double m_TStep;
    double m_RF_Freq;
    std::complex<double> m_RF_Gain;
    double m_IF_Freq;
    std::complex<double> m_IF_Gain;
    double m_IF_SamplingRate;
    double m_BandWidth;
    int    m_ADC_NBits;
    double m_PhaseImbalance;
    int    m_BB_DownSamplingRatio;
    double m_RC_ExcessBW;
    double m_Out_CenterFreq;

    // ========== 噪声参数 ==========
    double m_NoiseFigure_RFGain;
    double m_NoiseFigure_IFGain;
    double m_NoiseFigure_Mixer;

    // ========== RF 增益压缩参数 ==========
    int    m_GCType_RFGain;
    double m_TOIout_RFGain;
    double m_dBc1out_RFGain;
    double m_PSat_RFGain;
    double m_GCSat_RFGain;
    std::vector<double> m_GComp_RFGain_Data;

    // ========== IF 增益压缩参数 ==========
    int    m_GCType_IFGain;
    double m_TOIout_IFGain;
    double m_dBc1out_IFGain;
    double m_PSat_IFGain;
    double m_GCSat_IFGain;
    std::vector<double> m_GComp_IFGain_Data;

    // ========== 枚举解析 ==========
    static RADAR_Rx::SelectedGCType ConvertStringToGCType(const std::string& value);

    // ========== 运行时状态 ==========
    int m_outRate;

    // ========== 时间驱动缓冲队列 ==========
    std::queue<EnvelopeSignal> m_inputQueue;
    std::queue<std::complex<double>> m_outputQueue;
    int m_outputCount;
};

RegAlgo(RADAR_Rx_Block);

#endif // RADAR_RX_BLOCK_H
