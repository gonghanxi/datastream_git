#ifndef VARDELAYCX_BLOCK_H
#define VARDELAYCX_BLOCK_H
#include "VarDelayCx.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelayCx_Block : public Block
{
public:
    VarDelayCx_Block(const std::string& name);
    ~VarDelayCx_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelayCx> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;
};
RegAlgo(VarDelayCx_Block)

#endif // VARDELAYCX_BLOCK_H
