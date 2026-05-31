#ifndef REPEATCX_BLOCK_H
#define REPEATCX_BLOCK_H
#include "RepeatCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RepeatCx_Block : public Block
{
public:
    RepeatCx_Block(const std::string& name);
    ~RepeatCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<RepeatCx> m_Rep;

    double BlockSize;
    double NumTimes;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RepeatCx_Block);

#endif // REPEATCX_BLOCK_H
