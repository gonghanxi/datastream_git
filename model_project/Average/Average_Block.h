#ifndef AVERAGE_BLOCK_H
#define AVERAGE_BLOCK_H
#include "Average.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Average_Block : public Block
{
public:
    Average_Block(const std::string& name);
    ~Average_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<Average> m_Average;

    int m_NumInputsToAverage;
    int m_BlockSize;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Average_Block);
#endif // AVERAGE_BLOCK_H
