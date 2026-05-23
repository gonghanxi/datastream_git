#ifndef INVERSE_M_BLOCK_H
#define INVERSE_M_BLOCK_H

#include "Block.h"
#include "Inverse_M.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Inverse_M_Block : public SystemVueModelBuilder::Block
{
public:
    Inverse_M_Block(const std::string& name);
    ~Inverse_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:


    std::unique_ptr<Inverse_M> m_Inverse_M;
};
RegAlgo(Inverse_M_Block);

#endif // INVERSE_M_BLOCK_H
