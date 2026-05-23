#ifndef BITSTOINT_BLOCK_H
#define BITSTOINT_BLOCK_H

#include "BitsToInt.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BitsToInt_Block : public SystemVueModelBuilder::Block
{
public:
    BitsToInt_Block(const std::string& name);
    ~BitsToInt_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    BitsToInt::BitOrderEnum ConvertStringToBitOrder(const std::string& value);

    std::unique_ptr<BitsToInt> m_bitsToInt;

    int m_numBits;
    BitsToInt::BitOrderEnum m_bitOrder;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(BitsToInt_Block);

#endif // BITSTOINT_BLOCK_H
