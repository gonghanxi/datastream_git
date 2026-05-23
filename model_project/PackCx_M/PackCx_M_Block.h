#ifndef PACKCX_M_BLOCK_H
#define PACKCX_M_BLOCK_H

#include "Block.h"
#include "PackCx_M.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PackCx_M_Block : public Block
{
public:
    PackCx_M_Block(const std::string& name);\
    ~PackCx_M_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    PackCx_M::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);

    void SetDefaultParameters();

    PackCx_M::SelectedFormat m_Format;
    int m_NumRows;
    int m_NumCols;

    std::unique_ptr<PackCx_M> m_Pack_M;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<DComplexMatrix> m_outputQueue;    // 输出分发队列
    DComplexMatrix m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(PackCx_M_Block);

#endif // PACKCX_M_BLOCK_H
