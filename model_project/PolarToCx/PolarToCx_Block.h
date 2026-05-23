#pragma once
#include "PolarToCx.h"
#include "Block.h"
#include <string>
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PolarToCx_Block : public SystemVueModelBuilder::Block
{
public:
    PolarToCx_Block(const std::string& name);
    ~PolarToCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    std::unique_ptr<PolarToCx> m_polarToCx;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_magBuffer;   // 多输入累积缓冲区
    std::vector<double> m_phaseBuffer;
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(PolarToCx_Block);
