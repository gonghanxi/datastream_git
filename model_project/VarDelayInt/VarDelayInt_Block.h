#ifndef VARDELAYINT_BLOCK_H
#define VARDELAYINT_BLOCK_H
#include "VarDelayInt.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API VarDelayInt_Block : public Block
{
public:
    VarDelayInt_Block(const std::string& name);
    ~VarDelayInt_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<VarDelayInt> m_VarDelay;

    int MaxDelay;

    SystemVueModelBuilder::CircularBuffer<int> m_buffer;

    size_t m_iDelay;
    size_t m_iMaxDelay;
};
RegAlgo(VarDelayInt_Block)

#endif // VARDELAYINT_BLOCK_H
