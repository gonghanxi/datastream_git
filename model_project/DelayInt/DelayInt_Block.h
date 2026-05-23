#ifndef DELAYINT_BLOCK_H
#define DELAYINT_BLOCK_H

#include "Block.h"
#include "DelayInt.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DelayInt_Block : public SystemVueModelBuilder::Block
{
public:
    DelayInt_Block(const std::string& name);
    ~DelayInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    DelayInt::OutputTimingEnum ConvertStringToOutputTimingEnum(const std::string& value);
    void SetDefaultParamters();
    void SetParameters(int n, DelayInt::OutputTimingEnum timing);
    void ResetState();

    int m_n;
    DelayInt::OutputTimingEnum m_outputTiming;
    std::unique_ptr<DelayInt> m_delayInt;

    std::vector<int> m_buf;
    std::size_t m_head;
    int m_warmup;
};
RegAlgo(DelayInt_Block);
#endif // DELAYINT_BLOCK_H
