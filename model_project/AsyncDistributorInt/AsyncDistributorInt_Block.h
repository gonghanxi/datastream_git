#ifndef ASYNCDISTRIBUTORINT_BLOCK_H
#define ASYNCDISTRIBUTORINT_BLOCK_H
#include "AsyncDistributorInt.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AsyncDistributorInt_Block : public Block
{
public:
    AsyncDistributorInt_Block(const std::string& name);
    ~AsyncDistributorInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<AsyncDistributorInt> m_AsyncDistributor;

    Matrix<int> m_BlockSizes;
    std::vector<int> m_blockSizes;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<std::queue<int>> m_channelQueues;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AsyncDistributorInt_Block);

#endif // ASYNCDISTRIBUTORINT_BLOCK_H
