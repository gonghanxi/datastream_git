#ifndef CONVOLVECX_BLOCK_H
#define CONVOLVECX_BLOCK_H
#include "ConvolveCx.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API ConvolveCx_Block : public Block
{
public:
    ConvolveCx_Block(const std::string& name);
    ~ConvolveCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<ConvolveCx> m_Convolve;

    int m_TruncationDepth;

    std::vector< std::complex<double> > histA_;
    std::vector< std::complex<double> > histB_;

    std::size_t depth_;
    std::size_t sampleCount_;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inABuffer;   // 多输入累积缓冲区
    std::vector<std::complex<double>> m_inBBuffer;
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(ConvolveCx_Block);

#endif // CONVOLVECX_BLOCK_H
