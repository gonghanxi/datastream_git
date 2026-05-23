#ifndef ASYNCDISTRIBUTOR_BLOCK_H
#define ASYNCDISTRIBUTOR_BLOCK_H
#include "AsyncDistributor.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AsyncDistributor_Block : public Block
{
public:
    AsyncDistributor_Block(const std::string& name);
    ~AsyncDistributor_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<AsyncDistributor> m_AsyncDistributor;

    Matrix<int> m_BlockSizes;
    std::vector<int> m_blockSizes;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<std::queue<double>> m_channelQueues;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AsyncDistributor_Block);
#endif // ASYNCDISTRIBUTOR_BLOCK_H
