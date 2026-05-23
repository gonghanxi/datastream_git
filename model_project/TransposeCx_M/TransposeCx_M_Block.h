#ifndef TRANSPOSECX_M_BLOCK_H
#define TRANSPOSECX_M_BLOCK_H

#include "Block.h"
#include "TransposeCx_M.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TransposeCx_M_Block : public Block
{
public:
    TransposeCx_M_Block(const std::string& name);
    ~TransposeCx_M_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

private:
    std::unique_ptr<TransposeCx_M> m_Transpose_M;
};
RegAlgo(TransposeCx_M_Block);

#endif // TRANSPOSECX_M_BLOCK_H
