#ifndef TRANSPOSE_BLOCK_H
#define TRANSPOSE_BLOCK_H
#include "Block.h"
#include "Transpose.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Transpose_Block : public Block
{
public:
    Transpose_Block(const std::string& name);
    ~Transpose_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<Transpose> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Transpose_Block);

#endif // TRANSPOSE_BLOCK_H
