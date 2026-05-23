#ifndef PACK_M_BLOCK_H
#define PACK_M_BLOCK_H

#include "Block.h"
#include "Pack_M.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Pack_M_Block : public Block
{
public:
    Pack_M_Block(const std::string& name);
    ~Pack_M_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Pack_M::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);

    void SetDefaultParameters();

    Pack_M::SelectedFormat m_Format;
    int m_NumRows;
    int m_NumCols;

    std::unique_ptr<Pack_M> m_Pack_M;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<DoubleMatrix> m_outputQueue;    // 输出分发队列
    DoubleMatrix m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Pack_M_Block);

#endif // PACK_M_BLOCK_H
