#pragma once
#include "RADAR_MTI.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MTI_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MTI_Block(const std::string& name);
    ~RADAR_MTI_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double pri, double sampleRate, int numOfPulse, RADAR_MTI::SelectedMTI_Type mtiType);

private:
    RADAR_MTI::SelectedMTI_Type ConvertStringToMTIType(const std::string& value);
    void SetDefaultParamters();

    std::unique_ptr<RADAR_MTI> m_radarMti;

    double m_pri;
    double m_sampleRate;
    int m_numOfPulse;
    RADAR_MTI::SelectedMTI_Type m_mtiType;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_MTI_Block);
