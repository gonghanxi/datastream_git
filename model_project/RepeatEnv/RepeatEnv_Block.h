#ifndef REPEATENV_BLOCK_H
#define REPEATENV_BLOCK_H
#include "RepeatEnv.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RepeatEnv_Block : public Block
{
public:
    RepeatEnv_Block(const std::string& name);
    ~RepeatEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<RepeatEnv> m_Rep;

    double BlockSize;
    double NumTimes;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RepeatEnv_Block);

#endif // REPEATENV_BLOCK_H
