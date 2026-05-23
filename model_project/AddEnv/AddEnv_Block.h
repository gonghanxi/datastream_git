#ifndef ADDENV_BLOCK_H
#define ADDENV_BLOCK_H

#include "AddEnv.h"
#include "Block.h"
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AddEnv_Block : public SystemVueModelBuilder::Block
{
public:
    AddEnv_Block(const std::string& name);
    ~AddEnv_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double UserDefinedFc = 100e6, AddEnv::SelectedOutputFc OutputFc = AddEnv::center);

private:
    AddEnv::SelectedOutputFc ConvertStringToSelectedOutputFc(const std::string& value);
    unsigned long long GetCount() const { return m_firingCount; }

    void SetDefaultParameters();

    void PropagateCharacterizationFrequency();



    std::unique_ptr<AddEnv> m_addEnv;

    AddEnv::SelectedOutputFc	m_OutputFc;
    double	m_UserDefinedFc;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<EnvelopeSignal>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

    //
    SimuParameter simulator_param;
    double	fcOut;
    double	fc, fcmax, fcmin, fcmean;
    unsigned long long m_firingCount = 0;
};

RegAlgo(AddEnv_Block);

#endif // ADDENV_BLOCK_H
