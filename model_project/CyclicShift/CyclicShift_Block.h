#ifndef CYCLICSHIFT_BLOCK_H
#define CYCLICSHIFT_BLOCK_H
#include "CyclicShift.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API CyclicShift_Block : public Block
{
public:
    CyclicShift_Block(const std::string& name);
    ~CyclicShift_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<CyclicShift> m_CyclicShift;
    int m_BlockSize;
    int m_Offset;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(CyclicShift_Block);
#endif // CYCLICSHIFT_BLOCK_H
