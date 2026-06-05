#ifndef RADAR_ARRAYCOUPLE_BLOCK_H
#define RADAR_ARRAYCOUPLE_BLOCK_H

#include "Block.h"
#include "RADAR_ArrayCouple.h"

#include <complex>
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_ArrayCouple_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_ArrayCouple_Block(const std::string& name);
    ~RADAR_ArrayCouple_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_ArrayCouple> m_algo;

    // ===== 参数 =====
    int m_ChannelNum;
    SystemVueModelBuilder::Matrix<std::complex<double>> m_CoupleCoef;

    // ===== TimeDrivenRun 累积 =====
    std::vector<EnvelopeSignal> m_inputAccumulator;
    std::queue<std::vector<EnvelopeSignal>> m_outputQueue;
};

RegAlgo(RADAR_ArrayCouple_Block);

#endif // RADAR_ARRAYCOUPLE_BLOCK_H
