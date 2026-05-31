#ifndef UPSAMPLEENV_BLOCK_H
#define UPSAMPLEENV_BLOCK_H

#include "UpSampleEnv.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API UpSampleEnv_Block : public SystemVueModelBuilder::Block
{
public:
    UpSampleEnv_Block(const std::string& name);
    ~UpSampleEnv_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;



    void SetParameters(int factor = 5,
                       int phase = 0,
                       UpSampleEnv::ModeEnum mode = UpSampleEnv::Holdsample);
private:
    void UpdateCharacterizationFrequency();

    void SetDefaultParameters();

    UpSampleEnv::ModeEnum ConvertStringToModeEnum(const std::string& value);

    std::unique_ptr<UpSampleEnv> m_upsampleEnv;

    int m_factor;
    int m_phase;
    UpSampleEnv::ModeEnum m_mode;

    double FcOut;
    std::complex<double> m_prevEnv;
    SystemVueModelBuilder::EnvelopeCircularBuffer interpEnv;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(UpSampleEnv_Block);

#endif // UPSAMPLEENV_BLOCK_H
