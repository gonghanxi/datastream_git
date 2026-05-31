#ifndef TRANSPOSEENV_BLOCK_H
#define TRANSPOSEENV_BLOCK_H
#include "Block.h"
#include "TransposeEnv.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TransposeEnv_Block : public Block
{
public:
    TransposeEnv_Block(const std::string& name);
    ~TransposeEnv_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<TransposeEnv> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(TransposeEnv_Block);

#endif // TRANSPOSEENV_BLOCK_H
