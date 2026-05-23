#ifndef INTERLEAVEDEINTERLEAVE_BLOCK_H
#define INTERLEAVEDEINTERLEAVE_BLOCK_H
#include "InterleaveDeinterleave.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API InterleaveDeinterleave_Block : public Block
{
public:
    InterleaveDeinterleave_Block(const std::string& name);
    ~InterleaveDeinterleave_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<InterleaveDeinterleave> m_inter;

    int Rows;
    int Columns;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(InterleaveDeinterleave_Block);
#endif // INTERLEAVEDEINTERLEAVE_BLOCK_H
