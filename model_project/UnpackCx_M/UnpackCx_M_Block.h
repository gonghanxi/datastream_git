#ifndef UNPACKCX_M_BLOCK_H
#define UNPACKCX_M_BLOCK_H
#include "Block.h"
#include "UnpackCx_M.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API UnpackCx_M_Block : public Block
{
public:
    UnpackCx_M_Block(const std::string& name);
    ~UnpackCx_M_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    UnpackCx_M::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<UnpackCx_M> m_Unpack_M;

    int m_NumRows;
    int m_NumCols;
    UnpackCx_M::SelectedFormat m_Format;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<DComplexMatrix> m_inputBuffer;   // 输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(UnpackCx_M_Block);

#endif // UNPACKCX_M_BLOCK_H
