#ifndef RADAR_NONCOINTGR_BLOCK_H
#define RADAR_NONCOINTGR_BLOCK_H

#include "Block.h"
#include "RADAR_NonCoIntgr.h"

#include <complex>
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_NonCoIntgr_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_NonCoIntgr_Block(const std::string& name);
    ~RADAR_NonCoIntgr_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    void ComputeRates();

    std::unique_ptr<RADAR_NonCoIntgr> m_algo;

    // ===== 参数 =====
    double m_PRI_Or_WaveGate;
    int    m_Number;
    double m_SampleRate;

    // ===== 派生速率 =====
    int m_samplesPerPulse;
    int m_inputRate;
    int m_outputRate;

    // ===== TimeDrivenRun =====
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<double>                m_outputQueue;
};

RegAlgo(RADAR_NonCoIntgr_Block);

#endif // RADAR_NONCOINTGR_BLOCK_H
