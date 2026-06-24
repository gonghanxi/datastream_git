#ifndef RADAR_TX_BLOCK_H
#define RADAR_TX_BLOCK_H

#include "RADAR_Tx.h"
#include "Block.h"
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Tx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Tx_Block(const std::string& name);
    ~RADAR_Tx_Block();

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

    std::unique_ptr<RADAR_Tx> m_tx;

    // ========== 基本参数 ==========
    double m_TStep;
    double m_RF_Freq;
    double m_RF_Gain_Re;
    double m_RF_Gain_Im;
    double m_IF_Freq;
    double m_IF_Gain_Re;
    double m_IF_Gain_Im;
    double m_IF_SamplingRate;
    double m_BandWidth;
    double m_In_CenterFreq;
    int    m_BB_UpSamplingRatio;
    double m_RC_ExcessBW;
    double m_PhaseImbalance;
    int    m_DAC_NBits;
    int    m_DAC_UpSamplingRatio;

    // ========== 噪声参数 ==========
    double m_NoiseFigure_RF_Gain;
    double m_NoiseFigure_IF_Gain;
    double m_NoiseFigure_Mixer;

    // ========== RF 增益压缩参数 ==========
    int    m_GCType_RF_Gain;
    double m_TOIout_RF_Gain;
    double m_dBc1out_RF_Gain;
    double m_PSat_RF_Gain;
    double m_GCSat_RF_Gain;
    int    m_RappS_RF_Gain;
    std::vector<double> m_GComp_RF_Gain_Data;

    // ========== IF 增益压缩参数 ==========
    int    m_GCType_IF_Gain;
    double m_TOIout_IF_Gain;
    double m_dBc1out_IF_Gain;
    double m_PSat_IF_Gain;
    double m_GCSat_IF_Gain;
    int    m_RappS_IF_Gain;
    std::vector<double> m_GComp_IF_Gain_Data;

    // ========== 枚举解析 ==========
    static RADAR_Tx::SelectedGCType ConvertStringToGCType(const std::string& value);

    // ========== 运行时状态 ==========
    int m_outRate;

    // ========== 时间驱动缓冲队列 ==========
    std::queue<std::complex<double>> m_inputQueue;
    std::queue<EnvelopeSignal> m_outputQueue;
    int m_outputCount;
};

RegAlgo(RADAR_Tx_Block);

#endif // RADAR_TX_BLOCK_H
