#ifndef RADAR_TARGETTRACK_M_BLOCK_H
#define RADAR_TARGETTRACK_M_BLOCK_H
#include "RADAR_TargetTrack_M.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_TargetTrack_M_Block : public Block
{
public:
    RADAR_TargetTrack_M_Block(const std::string& name);
    ~RADAR_TargetTrack_M_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<RADAR_TargetTrack_M> m_radar;

    double PRI_Or_WaveGate;
    double TrackGate;
    double InitGateStartTime;
    double SampleRate;

    int PRINum;
    double GateStartTime;
    int numRows;
    int numCols;

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ========== 时间驱动缓冲队列 ==========
    std::vector<DComplexMatrix> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<BoolMatrix> m_istrackBuffer;

    std::queue<DComplexMatrix> m_outputQueue;    // 输出分发队列
    std::queue<DoubleMatrix> m_GateStartQueue;
    std::queue<DoubleMatrix> m_RangeQueue;

    DComplexMatrix m_lastOutput;                 // 上次输出值（用于保持）
    DoubleMatrix m_lastGateStart;
    DoubleMatrix m_lastRange;

    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RADAR_TargetTrack_M_Block);
#endif // RADAR_TARGETTRACK_M_BLOCK_H
