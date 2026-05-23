#ifndef COMMUTATORCX_BLOCK_H
#define COMMUTATORCX_BLOCK_H

#include "CommutatorCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CommutatorCx_Block : public SystemVueModelBuilder::Block
{
public:
    CommutatorCx_Block(const std::string& name);
    ~CommutatorCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    std::unique_ptr<CommutatorCx> m_commutatorCx;

    int m_blockSize = 1;
    size_t m_iBlockSize = 1U;

    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<std::complex<double>>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(CommutatorCx_Block);

#endif // COMMUTATORCX_BLOCK_H
