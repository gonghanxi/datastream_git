#ifndef INTTOBITS_BLOCK_H
#define INTTOBITS_BLOCK_H

#include "Block.h"
#include "IntToBits.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API IntToBits_Block : public SystemVueModelBuilder::Block
{
public:
    IntToBits_Block(const std::string& name);
    ~IntToBits_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    IntToBits::BitOrderEnum ConvertStringToBitOrder(const std::string& value);

    std::unique_ptr<IntToBits> m_IntTobits;

    int m_numBits;
    IntToBits::BitOrderEnum m_bitOrder;

    bool DataStreamRun(std::vector<int> outputData);
    bool TimeDrivenRun(std::vector<int> outputData);
    // ========== 时间驱动缓冲队列 ==========
    std::queue<int> m_outputQueue;
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(IntToBits_Block);

#endif // INTTOBITS_BLOCK_H
