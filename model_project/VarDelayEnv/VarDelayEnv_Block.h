#ifndef VARDELAYENV_BLOCK_H
#define VARDELAYENV_BLOCK_H
#include "VarDelayEnv.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelayEnv_Block : public Block
{
public:
    VarDelayEnv_Block(const std::string& name);
    ~VarDelayEnv_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelayEnv> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::EnvelopeCircularBuffer m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 输入累积缓冲区
    std::vector<int> m_controlBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(VarDelayEnv_Block)

#endif // VARDELAYENV_BLOCK_H
