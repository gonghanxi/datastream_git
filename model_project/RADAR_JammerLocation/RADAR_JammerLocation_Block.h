#ifndef RADAR_JAMMERLOCATION_BLOCK_H
#define RADAR_JAMMERLOCATION_BLOCK_H

#include "Block.h"
#include "RADAR_JammerLocation.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_JammerLocation_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_JammerLocation_Block(const std::string& name);
    ~RADAR_JammerLocation_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_JammerLocation> m_algo;

    // ===== 参数 =====
    double m_PRI;
    double m_SampleRate;

    // ===== 算法状态 =====
    int m_BufferSize;       // PRI * SampleRate, 一帧需要的采样点数

    // ===== TimeDrivenRun 累积 =====
    std::vector<double> m_inputAccumulator;
    std::queue<double>  m_outputQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;
};

RegAlgo(RADAR_JammerLocation_Block);

#endif // RADAR_JAMMERLOCATION_BLOCK_H
