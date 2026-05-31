#ifndef UNPACK_M_BLOCK_H
#define UNPACK_M_BLOCK_H

#include "Block.h"
#include "Unpack_M.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Unpack_M_Block : public Block
{
public:
    Unpack_M_Block(const std::string& name);
    ~Unpack_M_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    Unpack_M::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<Unpack_M> m_Unpack_M;

    int m_NumRows;
    int m_NumCols;
    Unpack_M::SelectedFormat m_Format;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<DoubleMatrix> m_inputBuffer;   // 输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Unpack_M_Block);
#endif // UNPACK_M_BLOCK_H
