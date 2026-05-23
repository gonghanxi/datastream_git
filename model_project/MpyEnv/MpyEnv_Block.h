#ifndef MPYENV_BLOCK_H
#define MPYENV_BLOCK_H

#include "MpyEnv.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MpyEnv_Block : public SystemVueModelBuilder::Block
{
public:
    MpyEnv_Block(const std::string& name);
    ~MpyEnv_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double userDefinedFc = 100e6, MpyEnv::SelectedFcOut fcOut = MpyEnv::center);

private:
    MpyEnv::SelectedFcOut ConvertStringToSelectedFcOut(const std::string& value);
    void SetDefaultParameters();
    void PropagateCharacterizationFrequency();

    std::unique_ptr<MpyEnv> m_mpyEnv;
    MpyEnv::SelectedFcOut m_FcOut;
    double m_UserDefinedFc;

    // 载频相关成员变量
    double fcOut;
    double fc, fcmax, fcmin, fcmean;

    SimuParameter simulator_param;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<EnvelopeSignal>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;    // 输出分发队列
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(MpyEnv_Block);

#endif // MPYENV_BLOCK_H
