#ifndef ADDINT_BLOCK_H
#define ADDINT_BLOCK_H

#include "Block.h"
#include "AddInt.h"
#include <queue>

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AddInt_Block : public SystemVueModelBuilder::Block
{
public:
    AddInt_Block(const std::string& name);
    ~AddInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();

    std::unique_ptr<AddInt> m_addInt;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<int>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AddInt_Block);
#endif // ADDINT_BLOCK_H
