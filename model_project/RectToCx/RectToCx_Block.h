#ifndef RECTTOCX_BLOCK_H
#define RECTTOCX_BLOCK_H
#include "RectToCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RectToCx_Block : public SystemVueModelBuilder::Block
{
public:
    RectToCx_Block(const std::string& name);
    ~RectToCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    int GetBatchSize() const override;
    int RunBatch(int maxCount) override;

private:

    std::unique_ptr<RectToCx> m_rectoCx;
    int m_batchSize = 10;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_realBuffer;   // 多输入累积缓冲区
    std::vector<double> m_imagBuffer;
    std::queue<std::complex<double>> m_OutputQueue;    // 输出分发队列

    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RectToCx_Block);
#endif // RECTTOCX_BLOCK_H
