#ifndef VARDELAYCX_BLOCK_H
#define VARDELAYCX_BLOCK_H
#include "VarDelayCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelayCx_Block : public Block
{
public:
    VarDelayCx_Block(const std::string& name);
    ~VarDelayCx_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelayCx> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 输入累积缓冲区
    std::vector<int> m_controlBuffer;
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(VarDelayCx_Block)

#endif // VARDELAYCX_BLOCK_H
