#pragma once
#include "RADAR_PulseCompression.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PulseCompression_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_PulseCompression_Block(const std::string& name);
    ~RADAR_PulseCompression_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(int samplenum, int fftSize, double bandwidth, double sampleRate,
                       RADAR_PulseCompression::SelectedWindowType windowType, double windowParameter);

private:
    RADAR_PulseCompression::SelectedWindowType ConvertStringToWindowType(const std::string& value);
    void SetDefaultParamters();

    std::unique_ptr<RADAR_PulseCompression> m_radarPulseCompression;

    int m_samplenum;
    int m_fftSize;
    double m_bandwidth;
    double m_sampleRate;
    RADAR_PulseCompression::SelectedWindowType m_windowType;
    double m_windowParameter;

    bool ProcessData(Matrix<std::complex<double>> fullSequence);
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

RegAlgo(RADAR_PulseCompression_Block);
