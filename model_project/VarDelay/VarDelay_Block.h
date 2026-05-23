#ifndef VARDELAY_BLOCK_H
#define VARDELAY_BLOCK_H
#include "VarDelay.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelay_Block : public Block
{
public:
    VarDelay_Block(const std::string& name);
    ~VarDelay_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelay> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::CircularBuffer<double> m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;
};
RegAlgo(VarDelay_Block)
#endif // VARDELAY_BLOCK_H
