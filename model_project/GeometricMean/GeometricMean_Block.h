#ifndef GEOMETRICMEAN_BLOCK_H
#define GEOMETRICMEAN_BLOCK_H
#include "GeometricMean.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API GeometricMean_Block : public Block
{
public:
    GeometricMean_Block(const std::string& name);
    ~GeometricMean_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<GeometricMean> m_GeometricMean;
    int m_N;
    double m_Gain;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(GeometricMean_Block);

#endif // GEOMETRICMEAN_BLOCK_H
