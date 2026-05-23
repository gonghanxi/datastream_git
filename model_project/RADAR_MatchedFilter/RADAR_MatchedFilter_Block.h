#pragma once
#include "RADAR_MatchedFilter.h"
#include "Block.h"
#include <complex>
#include <vector>
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MatchedFilter_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MatchedFilter_Block(const std::string& name);
    ~RADAR_MatchedFilter_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double pulseWidth, double pri, double sampleRate);

private:
    void SetDefaultParamters();

    std::unique_ptr<RADAR_MatchedFilter> m_radarMatchedFilter;

    double m_pulseWidth;
    double m_pri;
    double m_sampleRate;
    std::vector<std::complex<double>> m_cachedSignal;
    std::vector<std::complex<double>> m_cachedReference;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_signalBuffer;   // 多输入累积缓冲区
    std::vector<std::complex<double>> m_referenceBuffer;
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_MatchedFilter_Block);
