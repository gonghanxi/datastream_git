#ifndef CONVOLVE_BLOCK_H
#define CONVOLVE_BLOCK_H
#include "Convolve.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Convolve_Block : public Block
{
public:
    Convolve_Block(const std::string& name);
    ~Convolve_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<Convolve> m_Convolve;

    int m_TruncationDepth;

    std::vector<double> histA_;
    std::vector<double> histB_;

    unsigned long long iter_;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inABuffer;   // 多输入累积缓冲区
    std::vector<double> m_inBBuffer;
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Convolve_Block);

#endif // CONVOLVE_BLOCK_H
