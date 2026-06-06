#ifndef RADAR_SUMMERBUSRF_BLOCK_H
#define RADAR_SUMMERBUSRF_BLOCK_H

#include "Block.h"
#include "RADAR_SummerBusRF.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_SummerBusRF_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_SummerBusRF_Block(const std::string& name);
    ~RADAR_SummerBusRF_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    int  ConvertStringToFcOut(const std::string& value);

    std::unique_ptr<RADAR_SummerBusRF> m_algo;

    // ===== 参数 =====
    int m_FcOut;  // 0=min, 1=center, 2=max (RADAR_SummerBusRF::SelectedFcOut 是 private enum)

    SimuParameter simulator_param;

    // ===== TimeDrivenRun =====
    std::vector<EnvelopeSignal>           m_input1Accumulator;
    std::vector<EnvelopeSignal>           m_input2Accumulator;
    std::queue<std::vector<EnvelopeSignal>> m_outputQueue;
};

RegAlgo(RADAR_SummerBusRF_Block);

#endif // RADAR_SUMMERBUSRF_BLOCK_H
