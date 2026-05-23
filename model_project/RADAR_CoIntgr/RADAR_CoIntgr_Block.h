#pragma once
#include "RADAR_CoIntgr.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CoIntgr_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_CoIntgr_Block(const std::string& name);
    ~RADAR_CoIntgr_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double priOrWaveGate, int numOfPulse, double sampleRate);

private:
    void SetDefaultParamters();

    std::unique_ptr<RADAR_CoIntgr> m_radarCoIntgr;

    double m_priOrWaveGate;
    int m_numOfPulse;
    double m_sampleRate;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_CoIntgr_Block);
