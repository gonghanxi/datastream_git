#ifndef DEMUX_BLOCK_H
#define DEMUX_BLOCK_H
#include "DeMux.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DeMux_Block : public Block
{
public:
    DeMux_Block(const std::string& name);
    ~DeMux_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<DeMux> m_DeMux;

    double m_BlockSize;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<int> m_controlBuffer;
    std::queue<double> m_outputQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(DeMux_Block);
#endif // DEMUX_BLOCK_H
