#ifndef INTERLEAVEDEINTERLEAVECX_BLOCK_H
#define INTERLEAVEDEINTERLEAVECX_BLOCK_H
#include "InterleaveDeinterleaveCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API InterleaveDeinterleaveCx_Block : public Block
{
public:
    InterleaveDeinterleaveCx_Block(const std::string& name);
    ~InterleaveDeinterleaveCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<InterleaveDeinterleaveCx> m_inter;

    int Rows;
    int Columns;

    unsigned m_blockSize;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(InterleaveDeinterleaveCx_Block);

#endif // INTERLEAVEDEINTERLEAVECX_BLOCK_H
