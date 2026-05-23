#ifndef RADAR_TARGETDETECT_BLOCK_H
#define RADAR_TARGETDETECT_BLOCK_H
#include "RADAR_TargetDetect.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_TargetDetect_Block : public Block
{
public:
    RADAR_TargetDetect_Block(const std::string& name);
    ~RADAR_TargetDetect_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();
    RADAR_TargetDetect::SelectedDetectType ConvertStringToSelectedDetectType(const std::string& value);

    std::unique_ptr<RADAR_TargetDetect> m_radar;

    // Parameter
    double PRI_Or_WaveGate;
    RADAR_TargetDetect::SelectedDetectType DetectType;
    double FalseAlarmProbability;
    int ReferenceCell;
    int GuardCell;
    int FreqChannelNum;
    double SampleRate;

    int CellSize;
    int PRINum;
    double Threshold;
    bool DetectStatus;

    bool DataStreamRun();
    bool TimeDrivenRun();
    bool ProcessData(std::vector<std::complex<double>> inputData);
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::queue<bool> m_IsDetectQueue;
    std::queue<int> m_RangeBinIndexQueue;
    std::queue<int> m_FreqBinIndexQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    double m_lastdetect;
    int m_lastrangeIndex;
    int m_lastfreqIndex;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RADAR_TargetDetect_Block);
#endif // RADAR_TARGETDETECT_BLOCK_H
