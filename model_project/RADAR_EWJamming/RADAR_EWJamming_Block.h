#ifndef RADAR_EWJAMMING_BLOCK_H
#define RADAR_EWJAMMING_BLOCK_H

#include "Block.h"
#include "RADAR_EWJamming.h"

#include <complex>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_EWJamming_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_EWJamming_Block(const std::string& name);
    ~RADAR_EWJamming_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_EWJamming> m_algo;

    // ---- parameters ----
    int    m_SampleNum;
    double m_SampleRate;
    double m_Mean;
    double m_Stdev;
    double m_System_Loss;

    // ---- random generator (Block 层维护) ----
    std::mt19937 m_rng;

    // ===== TimeDrivenRun 输出队列 =====
    std::queue<std::complex<double>> m_outputQueue;
};

RegAlgo(RADAR_EWJamming_Block);

#endif // RADAR_EWJAMMING_BLOCK_H
