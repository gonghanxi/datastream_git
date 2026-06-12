#ifndef RADAR_TX_DBS_MEASUREMENT_BLOCK_H
#define RADAR_TX_DBS_MEASUREMENT_BLOCK_H

#include "Block.h"
#include "RADAR_Tx_DBS_Measurement.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Tx_DBS_Measurement_Block : public Block
{
public:
    typedef std::complex<double> Cx;

    RADAR_Tx_DBS_Measurement_Block(const std::string& name);
    ~RADAR_Tx_DBS_Measurement_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool validateAndPrepare();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // Advance：TimedDFModel 需要驱动时间轴
    void Advance();

    // 字符串 → 枚举转换
    static RADAR_Tx_DBS_Measurement::ParamToSweepEnum ConvertStringToParamToSweep(const std::string& value);
    static RADAR_Tx_DBS_Measurement::TypeOfSweepEnum  ConvertStringToTypeOfSweep(const std::string& value);

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_Tx_DBS_Measurement> m_algo;

    // ---- 参数 (统一使用原算法 public 枚举) ----
    double m_PRI;
    double m_SamplingRate;

    int m_NumOfAntx;
    int m_NumOfAnty;

    double m_Dx;
    double m_Dy;

    RADAR_Tx_DBS_Measurement::ParamToSweepEnum m_ParamToSweep;
    double m_Theta_Phi;

    RADAR_Tx_DBS_Measurement::TypeOfSweepEnum m_TypeOfSweep;
    double m_SweepStart;
    double m_SweepStop;
    int    m_SweepNumOfPoints;
    double m_SweepStepSize;

    // ---- 派生量 ----
    int m_nAnt;
    int m_inputRate;
    int m_sweepSamples;
    double m_sweepStepRad;

    // ---- TimeDrivenRun 缓冲区 (envelope 输入 + complex 输出) ----
    std::vector<EnvelopeSignal>   m_inputBusBuffer;  // 累积输入 envelope (flat: lane0_sample0..lane0_sampleR-1, lane1_sample0..)
    std::queue<Cx>                m_outputQueue;
};

RegAlgo(RADAR_Tx_DBS_Measurement_Block);

#endif // RADAR_TX_DBS_MEASUREMENT_BLOCK_H
