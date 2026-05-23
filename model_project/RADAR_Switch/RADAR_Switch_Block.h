#ifndef RADAR_SWITCH_BLOCK_H
#define RADAR_SWITCH_BLOCK_H

#include "RADAR_Switch.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Switch_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Switch_Block(const std::string& name);
    ~RADAR_Switch_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();


    std::unique_ptr<RADAR_Switch> m_radarSwitch;

    double m_PRF;
    double m_SwitchOff_Time;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<double> m_RPIBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_Switch_Block);

#endif // RADAR_SWITCH_BLOCK_H
