#ifndef VARDELAYINT_BLOCK_H
#define VARDELAYINT_BLOCK_H
#include "VarDelayInt.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelayInt_Block : public Block
{
public:
    VarDelayInt_Block(const std::string& name);
    ~VarDelayInt_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelayInt> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::CircularBuffer<int> m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 输入累积缓冲区
    std::vector<int> m_controlBuffer;
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(VarDelayInt_Block)

#endif // VARDELAYINT_BLOCK_H
