#ifndef ADDENV_M_BLOCK_H
#define ADDENV_M_BLOCK_H
#include "AddEnv_M.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AddEnv_M_Block : public Block
{
public:
    AddEnv_M_Block(const std::string& name);
    ~AddEnv_M_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    std::unique_ptr<AddEnv_M> m_add;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<EnvelopeMatrix>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeMatrix> m_outputQueue;    // 输出分发队列
    EnvelopeMatrix m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

    int ChannelNum;
};
RegAlgo(AddEnv_M_Block)
#endif // ADDENV_M_BLOCK_H
