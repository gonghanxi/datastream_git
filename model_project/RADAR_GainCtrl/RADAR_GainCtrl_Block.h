#ifndef RADAR_GAINCTRL_BLOCK_H
#define RADAR_GAINCTRL_BLOCK_H

#include "RADAR_GainCtrl.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_GainCtrl_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_GainCtrl_Block(const std::string& name);
    ~RADAR_GainCtrl_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RADAR_GainCtrl::SelectedControlType ConvertStringToSelectedControlType(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<RADAR_GainCtrl> m_radarGainCtrl;

    RADAR_GainCtrl::SelectedControlType m_ControlType;
    double m_PRI;
    double m_Gain;
    double m_STC_Factor;
    double m_STC_StartTime;
    double m_STC_StopTime;
    double m_STC_K_Coef;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<double> m_gainBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_GainCtrl_Block);

#endif // RADAR_GAINCTRL_BLOCK_H
