#ifndef ASYNCCOMMUTATORENV_BLOCK_H
#define ASYNCCOMMUTATORENV_BLOCK_H

#include "Block.h"
#include "AsyncCommutatorEnv.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AsyncCommutatorEnv_Block : public SystemVueModelBuilder::Block
{
public:
    AsyncCommutatorEnv_Block(const std::string& name);
    ~AsyncCommutatorEnv_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    bool UpdateCharacterizationFrequency();

    std::unique_ptr<AsyncCommutatorEnv> m_asyncCommutatorEnv;
    SystemVueModelBuilder::Matrix<int> m_blockSizes;

    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<EnvelopeSignal>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AsyncCommutatorEnv_Block);
#endif // ASYNCCOMMUTATORENV_BLOCK_H
