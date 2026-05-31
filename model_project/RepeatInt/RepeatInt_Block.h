#ifndef REPEATINT_BLOCK_H
#define REPEATINT_BLOCK_H
#include "RepeatInt.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RepeatInt_Block : public Block
{
public:
    RepeatInt_Block(const std::string& name);
    ~RepeatInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<RepeatInt> m_Rep;

    double BlockSize;
    double NumTimes;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RepeatInt_Block);

#endif // REPEATINT_BLOCK_H
