#ifndef VARDELAYENV_BLOCK_H
#define VARDELAYENV_BLOCK_H
#include "VarDelayEnv.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelayEnv_Block : public Block
{
public:
    VarDelayEnv_Block(const std::string& name);
    ~VarDelayEnv_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelayEnv> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::EnvelopeCircularBuffer m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;
};
RegAlgo(VarDelayEnv_Block)

#endif // VARDELAYENV_BLOCK_H
