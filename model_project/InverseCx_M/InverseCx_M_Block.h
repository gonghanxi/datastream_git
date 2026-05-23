#ifndef INVERSECX_M_BLOCK_H
#define INVERSECX_M_BLOCK_H

#include "Block.h"
#include "InverseCx_M.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API InverseCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    InverseCx_M_Block(const std::string& name);
    ~InverseCx_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:


    std::unique_ptr<InverseCx_M> m_InverseCx_M;
};
RegAlgo(InverseCx_M_Block);
#endif // INVERSECX_M_BLOCK_H
