#ifndef COMMUTATOR_BLOCK_H
#define COMMUTATOR_BLOCK_H

#include "Block.h"
#include "Commutator.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Commutator_Block : public SystemVueModelBuilder::Block
{
public:
    Commutator_Block(const std::string& name);
    ~Commutator_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();


    std::unique_ptr<Commutator> m_commutator;

    int m_blockSize;
    size_t m_iBlockSize;

    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(Commutator_Block);

#endif // COMMUTATOR_BLOCK_H
