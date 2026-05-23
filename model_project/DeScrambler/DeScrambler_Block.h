#ifndef DESCRAMBLER_BLOCK_H
#define DESCRAMBLER_BLOCK_H
#include "DeScrambler.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DeScrambler_Block : public Block
{
public:
    DeScrambler_Block(const std::string& name);
    ~DeScrambler_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<DeScrambler> m_de;

    int Polynomial;
    int ShiftReg;
};
RegAlgo(DeScrambler_Block);
#endif // DESCRAMBLER_BLOCK_H
