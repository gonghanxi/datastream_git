#ifndef ASYNCDISTRIBUTORCX_BLOCK_H
#define ASYNCDISTRIBUTORCX_BLOCK_H
#include "AsyncDistributorCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AsyncDistributorCx_Block : public Block
{
public:
    AsyncDistributorCx_Block(const std::string& name);
    ~AsyncDistributorCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<AsyncDistributorCx> m_AsyncDistributor;

    Matrix<int> m_BlockSizes;
    std::vector<int> m_blockSizes;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<std::queue<std::complex<double>>> m_channelQueues;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AsyncDistributorCx_Block);

#endif // ASYNCDISTRIBUTORCX_BLOCK_H
