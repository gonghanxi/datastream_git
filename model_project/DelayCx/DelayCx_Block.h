#ifndef DELAYCX_BLOCK_H
#define DELAYCX_BLOCK_H

#include "Block.h"
#include "DelayCx.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DelayCx_Block : public SystemVueModelBuilder::Block
{
public:
    DelayCx_Block(const std::string& name);
    ~DelayCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    DelayCx::OutputTimingEnum ConvertStringToOutputTimingEnum(const std::string& value);
    void SetDefaultParamters();
    void SetParameters(int n, DelayCx::OutputTimingEnum timing);
    void ResetState();

    int m_n;
    DelayCx::OutputTimingEnum m_outputTiming;
    std::unique_ptr<DelayCx> m_delayCx;

    std::vector<std::complex<double>> m_buf;
    std::size_t m_head;
    int m_warmup;
};
RegAlgo(DelayCx_Block);
#endif // DELAYCX_BLOCK_H
