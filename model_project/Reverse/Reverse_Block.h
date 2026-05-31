#ifndef REVERSE_BLOCK_H
#define REVERSE_BLOCK_H

#include "Reverse.h"
#include "Block.h"
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Reverse_Block : public SystemVueModelBuilder::Block
{
public:
    Reverse_Block(const std::string& name);
    ~Reverse_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;


    void SetParameter(int n = 64);

    // ========== 变步长接口重写 ==========
    double GetMinimumTimeStep() const override;
    bool ShouldExecuteAt(double time) const override;
    int GetOutputDataCount() const override;
    int GetInputAccumulateCount() const override;

private:
    void SetDefaultParameters();

    std::unique_ptr<Reverse> m_reverse;
    int m_N;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

    // ========== 时间驱动配置 ==========
    double m_samplePeriod;               // 模型采样周期
};

RegAlgo(Reverse_Block);
#endif // REVERSE_BLOCK_H
