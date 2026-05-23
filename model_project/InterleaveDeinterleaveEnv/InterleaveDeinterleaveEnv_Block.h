#ifndef INTERLEAVEDEINTERLEAVEENV_BLOCK_H
#define INTERLEAVEDEINTERLEAVEENV_BLOCK_H
#include "InterleaveDeinterleaveEnv.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API InterleaveDeinterleaveEnv_Block : public Block
{
public:
    InterleaveDeinterleaveEnv_Block(const std::string& name);
    ~InterleaveDeinterleaveEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<InterleaveDeinterleaveEnv> m_inter;

    int Rows;
    int Columns;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(InterleaveDeinterleaveEnv_Block);

#endif // INTERLEAVEDEINTERLEAVEENV_BLOCK_H
