#ifndef CYCLICSHIFTCX_BLOCK_H
#define CYCLICSHIFTCX_BLOCK_H
#include "CyclicShiftCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API CyclicShiftCx_Block : public Block
{
public:
    CyclicShiftCx_Block(const std::string& name);
    ~CyclicShiftCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<CyclicShiftCx> m_CyclicShift;
    int m_BlockSize;
    int m_Offset;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

};
RegAlgo(CyclicShiftCx_Block);

#endif // CYCLICSHIFTCX_BLOCK_H
