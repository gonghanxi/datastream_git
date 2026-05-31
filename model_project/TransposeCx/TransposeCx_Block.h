#ifndef TRANSPOSECX_BLOCK_H
#define TRANSPOSECX_BLOCK_H
#include "Block.h"
#include "TransposeCx.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TransposeCx_Block : public Block
{
public:
    TransposeCx_Block(const std::string& name);
    ~TransposeCx_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<TransposeCx> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(TransposeCx_Block);

#endif // TRANSPOSECX_BLOCK_H
