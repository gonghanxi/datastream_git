#ifndef BITSHIFTREGISTER_BLOCK_H
#define BITSHIFTREGISTER_BLOCK_H

#include "BitShiftRegister.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BitShiftRegister_Block : public SystemVueModelBuilder::Block
{
public:
    BitShiftRegister_Block(const std::string& name);
    ~BitShiftRegister_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    BitShiftRegister::BitOrderEnum ConvertStringToBitOrder(const std::string& value);

    std::unique_ptr<BitShiftRegister> m_bitShiftRegister;

    int m_numBits;
    BitShiftRegister::BitOrderEnum m_bitOrder;
    std::vector<int> m_reg;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<int> m_clockBuffer;
    std::vector<int> m_resetBuffer;
    std::queue<bool> m_outputQueue;    // 输出分发队列
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(BitShiftRegister_Block);

#endif // BITSHIFTREGISTER_BLOCK_H
