#ifndef HERMITIAN_M_BLOCK_H
#define HERMITIAN_M_BLOCK_H

#include "Block.h"
#include "Hermitian_M.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Hermitian_M_Block : public SystemVueModelBuilder::Block
{
public:
    Hermitian_M_Block(const std::string& name);
    ~Hermitian_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:

    std::unique_ptr<Hermitian_M> m_Hermitian_M;
};
RegAlgo(Hermitian_M_Block);
#endif // HERMITIAN_M_BLOCK_H
