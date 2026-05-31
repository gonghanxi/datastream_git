#ifndef SUBINT_BLOCK_H
#define SUBINT_BLOCK_H

#include "Block.h"
#include "SubInt.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SubInt_Block : public SystemVueModelBuilder::Block
{
public:
    SubInt_Block(const std::string& name);
    ~SubInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();

    std::unique_ptr<SubInt> m_subInt;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_posBuffer;
    std::map<BufferReader*, std::vector<int>> m_negBuffer;
    std::queue<int> m_outputQueue;
    double m_lastOutput;
    int m_inputCount;
    int m_outputCount;
};
RegAlgo(SubInt_Block);
#endif // SUBINT_BLOCK_H
