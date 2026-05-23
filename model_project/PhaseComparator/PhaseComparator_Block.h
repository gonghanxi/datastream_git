#ifndef PHASECOMPARATOR_BLOCK_H
#define PHASECOMPARATOR_BLOCK_H
#include "PhaseComparator.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PhaseComparator_Block : public Block
{
public:
    PhaseComparator_Block(const std::string& name);
    ~PhaseComparator_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    PhaseComparator::PhaseCharacteristicTypeEnum ConvertStringToPhaseCharacteristicTypeEnum(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<PhaseComparator> m_phase;

    PhaseComparator::PhaseCharacteristicTypeEnum PhaseCharacteristicType;
    double GainConstant;
    double MaxAngle;

    SimuParameter simulator_param;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_s1Buffer;   // 多输入累积缓冲区
    std::vector<EnvelopeSignal> m_s2Buffer;
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(PhaseComparator_Block);
#endif // PHASECOMPARATOR_BLOCK_H
