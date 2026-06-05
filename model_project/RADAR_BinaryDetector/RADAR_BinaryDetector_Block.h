#ifndef RADAR_BINARYDETECTOR_BLOCK_H
#define RADAR_BINARYDETECTOR_BLOCK_H

#include "Block.h"
#include "RADAR_BinaryDetector.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_BinaryDetector_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_BinaryDetector_Block(const std::string& name);
    ~RADAR_BinaryDetector_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_BinaryDetector> m_algo;

    // ===== 参数 =====
    double m_Threshold;
    double m_PRI;
    double m_SampleRate;

    // ===== 算法状态 =====
    int m_BufferSize;       // PRI * SampleRate, 一帧需要的采样点数

    // ===== TimeDrivenRun 累积 =====
    std::vector<double> m_inputAccumulator;
    std::queue<std::vector<int>> m_outputQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;
};

RegAlgo(RADAR_BinaryDetector_Block);

#endif // RADAR_BINARYDETECTOR_BLOCK_H
