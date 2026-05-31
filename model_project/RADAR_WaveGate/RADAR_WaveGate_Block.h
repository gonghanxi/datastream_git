#ifndef RADAR_WAVEGATE_BLOCK_H
#define RADAR_WAVEGATE_BLOCK_H
#include "RADAR_WaveGate.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_WaveGate_Block : public Block
{
public:
    RADAR_WaveGate_Block(const std::string& name);
    ~RADAR_WaveGate_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_WaveGate> m_radar;

    double PRF;
    double StartTime;
    double GateTime;
    double SampleRate;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<double> m_GCBuffer;
    std::queue<std::complex<double>> m_OutputQueue;    // 输出分发队列

    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RADAR_WaveGate_Block);
#endif // RADAR_WAVEGATE_BLOCK_H
