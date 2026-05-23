#ifndef RADAR_TARGETTRACK_BLOCK_H
#define RADAR_TARGETTRACK_BLOCK_H
#include "RADAR_TargetTrack.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_TargetTrack_Block : public Block
{
public:
    RADAR_TargetTrack_Block(const std::string& name);
    ~RADAR_TargetTrack_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<RADAR_TargetTrack> m_radar;

    double PRI_Or_WaveGate;
    double TrackGate;
    double InitGateStartTime;
    double SampleRate;

    int PRINum;
    double GateStartTime;

    bool DataStreamRun();
    bool TimeDrivenRun();
    bool ProcessData(std::vector<std::complex<double>> inputData);
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<bool> m_istrackBuffer;

    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::queue<double> m_GateStartQueue;
    std::queue<double> m_RangeQueue;

    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    double m_lastGateStart;
    double m_lastRange;

    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

};
RegAlgo(RADAR_TargetTrack_Block);
#endif // RADAR_TARGETTRACK_BLOCK_H
