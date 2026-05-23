#ifndef ASYNCDISTRIBUTORENV_BLOCK_H
#define ASYNCDISTRIBUTORENV_BLOCK_H
#include "AsyncDistributorEnv.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AsyncDistributorEnv_Block : public Block
{
public:
    AsyncDistributorEnv_Block(const std::string& name);
    ~AsyncDistributorEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<AsyncDistributorEnv> m_AsyncDistributor;

    Matrix<int> m_BlockSizes;
    std::vector<int> m_blockSizes;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<std::queue<EnvelopeSignal>> m_channelQueues;// 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AsyncDistributorEnv_Block);

#endif // ASYNCDISTRIBUTORENV_BLOCK_H
