#ifndef POLARTORECT_BLOCK_H
#define POLARTORECT_BLOCK_H
#include "PolarToRect.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PolarToRect_Block : public Block
{
public:
    PolarToRect_Block(const std::string& name);
    ~PolarToRect_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:
    std::unique_ptr<PolarToRect> m_PolarToRect;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_magBuffer;   // 多输入累积缓冲区
    std::vector<double> m_phaseBuffer;
    std::queue<double> m_xQueue;    // 输出分发队列
    std::queue<double> m_yQueue;
    double m_lastx;                 // 上次输出值（用于保持）
    double m_lasty;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(PolarToRect_Block);
#endif // POLARTORECT_BLOCK_H
