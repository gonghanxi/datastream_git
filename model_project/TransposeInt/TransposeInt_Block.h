#ifndef TRANSPOSEINT_BLOCK_H
#define TRANSPOSEINT_BLOCK_H
#include "Block.h"
#include "TransposeInt.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TransposeInt_Block : public Block
{
public:
    TransposeInt_Block(const std::string& name);
    ~TransposeInt_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<TransposeInt> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(TransposeInt_Block);

#endif // TRANSPOSEINT_BLOCK_H
