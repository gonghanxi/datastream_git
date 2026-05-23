#ifndef MIXER_BLOCK_H
#define MIXER_BLOCK_H

#include "Mixer.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Mixer_Block : public SystemVueModelBuilder::Block
{
public:
    Mixer_Block(const std::string& name);
    ~Mixer_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    bool UpdateCharacterizationFrequency(double& fcOut);

    Mixer::EnableNoiseEnum ConvertStringToEnableNoise(const std::string& value);
    Mixer::SidebandEnum ConvertStringToSideband(const std::string& value);

    std::unique_ptr<Mixer> m_mixer;

    double m_convGain;
    Mixer::EnableNoiseEnum m_enableNoise;
    double m_noiseFigure;
    Mixer::SidebandEnum m_sideband;
    double m_sidebandSuppression;
    double m_rfRej;
    double m_loRej;
    double m_loRfIso;
    double m_rfLoIso;
    double m_soiOut;
    double m_toiOut;
    double m_refR;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inBuffer;   // 多输入累积缓冲区
    std::vector<EnvelopeSignal> m_loBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(Mixer_Block);

#endif // MIXER_BLOCK_H
