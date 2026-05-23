#ifndef DELAYENV_BLOCK_H
#define DELAYENV_BLOCK_H

#include "Block.h"
#include "DelayEnv.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DelayEnv_Block : public SystemVueModelBuilder::Block
{
public:
    DelayEnv_Block(const std::string& name);
    ~DelayEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    DelayEnv::OutputTimingEnum ConvertStringToOutputTimingEnum(const std::string& value);
    void SetDefaultParamters();
    void SetParameters(int n, DelayEnv::OutputTimingEnum timing);
    void ResetState();
    void UpdateCharacterizationFrequency();

    int m_n;
    DelayEnv::OutputTimingEnum m_outputTiming;
    std::unique_ptr<DelayEnv> m_delayEnv;

    std::vector<SystemVueModelBuilder::EnvelopeSignal> m_buf;
    std::size_t m_head;
    int m_warmup;
};
RegAlgo(DelayEnv_Block);
#endif // DELAYENV_BLOCK_H
