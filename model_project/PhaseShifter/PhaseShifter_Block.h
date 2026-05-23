#ifndef PHASESHIFTER_BLOCK_H
#define PHASESHIFTER_BLOCK_H

#include "Block.h"
#include "PhaseShifter.h"
#include <deque>
#include <random>
#include <vector>
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PhaseShifter_Block : public SystemVueModelBuilder::Block
{
public:
    PhaseShifter_Block(const std::string& name);
    ~PhaseShifter_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    PhaseShifter::QuantEnum ConvertStringToQuantEnum(const std::string& value);
    PhaseShifter::ErrEnum ConvertStringToErrEnum(const std::string& value);

    void buildHilbert(int L);
    double hilbertConv() const;
    double delayedReal() const;
    double computePhaseRad(double baseDeg);
    double ampScale() const;

    bool UpdateCharacterizationFrequency(double& fc);

    std::unique_ptr<PhaseShifter> m_phaseShifter;

    double m_phaseShift;
    double m_insertionLoss;
    PhaseShifter::QuantEnum m_quantization;
    int m_numBits;
    SystemVueModelBuilder::Matrix<double> m_levels;

    PhaseShifter::ErrEnum m_phaseShiftError;
    double m_customError;
    double m_stdDev;
    double m_min;
    double m_max;

    double m_sensitivity;
    int m_hilbertFilterLength;

    static constexpr double kPI = 3.14159265358979323846;

    int m_L;
    std::vector<double> m_h;
    std::deque<double> m_x;

    std::mt19937 m_rngN;
    std::mt19937 m_rngU;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<double> m_controlBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(PhaseShifter_Block);

#endif // PHASESHIFTER_BLOCK_H
