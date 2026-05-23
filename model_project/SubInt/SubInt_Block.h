#ifndef SUBINT_BLOCK_H
#define SUBINT_BLOCK_H

#include "Block.h"
#include "SubInt.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SubInt_Block : public SystemVueModelBuilder::Block
{
public:
    SubInt_Block(const std::string& name);
    ~SubInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();

    std::unique_ptr<SubInt> m_subInt;
};
RegAlgo(SubInt_Block);
#endif // SUBINT_BLOCK_H
