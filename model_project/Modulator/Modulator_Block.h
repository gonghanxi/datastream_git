#ifndef MODULATOR_BLOCK_H
#define MODULATOR_BLOCK_H

#include "Modulator.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Modulator_Block : public SystemVueModelBuilder::Block
{
public:
    Modulator_Block(const std::string& name);
    ~Modulator_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(Modulator::InputTypeEnum inputype = Modulator::InIQ,
                       double fcarrier = 0.2e6,
                       double initialphase = 0,
                       double ampsensitivity = 1,
                       double phasesensitivity = 90,
                       double freqsensitivity = 10000,
                       Modulator::ConjQuadEnum conjugatedquadrature = Modulator::CQ_No,
                       Modulator::MirrorEnum mirrorsignal = Modulator::Mirror_No,
                       Modulator::ShowIQEnum showiq_impairments = Modulator::ShowIQ_NO,
                       double gainimbalance = 0.0,
                       double phaseimbalance = 0.0,
                       double i_originoffset = 0.0,
                       double q_originoffset = 0.0,
                       double iq_rotation = 0.0
                       );
private:
    Modulator::InputTypeEnum ConvertStringToInputTypeEnum(const std::string& value);
    Modulator::ConjQuadEnum ConvertStringToConjQuadEnum(const std::string& value);
    Modulator::MirrorEnum ConvertStringToMirrorEnum(const std::string& value);
    Modulator::ShowIQEnum ConvertStringToShowIQEnum(const std::string& value);
    void PropagateCharacterizationFrequency();
    void SetDefaultParameters();

    std::unique_ptr<Modulator> m_modulator;

    Modulator::InputTypeEnum  m_InputType;
    double         m_FCarrier;
    double         m_InitialPhase;
    double         m_AmpSensitivity;
    double         m_PhaseSensitivity;
    double         m_FreqSensitivity;
    Modulator::ConjQuadEnum   m_ConjugatedQuadrature;
    Modulator::MirrorEnum     m_MirrorSignal;
    Modulator::ShowIQEnum     m_ShowIQ_Impairments;

    double m_GainImbalance;
    double m_PhaseImbalance;
    double m_I_OriginOffset;
    double m_Q_OriginOffset;
    double m_IQ_Rotation;

    //
    SimuParameter simulator_param;
    double m_lastTime = std::numeric_limits<double>::quiet_NaN();

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_input1Buffer;   // 多输入累积缓冲区
    std::vector<double> m_input2Buffer;
    std::vector<EnvelopeSignal> m_loBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;
    std::queue<EnvelopeSignal> m_quad_outputQueue;
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    EnvelopeSignal m_lastQuad_Output;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(Modulator_Block);

#endif // MODULATOR_BLOCK_H
