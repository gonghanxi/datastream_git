#ifndef INTERLEAVEDEINTERLEAVEINT_BLOCK_H
#define INTERLEAVEDEINTERLEAVEINT_BLOCK_H
#include "InterleaveDeinterleaveInt.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API InterleaveDeinterleaveInt_Block : public Block
{
public:
    InterleaveDeinterleaveInt_Block(const std::string& name);
    ~InterleaveDeinterleaveInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<InterleaveDeinterleaveInt> m_inter;

    int Rows;
    int Columns;

    unsigned m_blockSize;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(InterleaveDeinterleaveInt_Block);

#endif // INTERLEAVEDEINTERLEAVEINT_BLOCK_H
