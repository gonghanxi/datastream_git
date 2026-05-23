#ifndef TRANSPOSE_M_BLOCK_H
#define TRANSPOSE_M_BLOCK_H

#include "Block.h"
#include "Transpose_M.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Transpose_M_Block : public Block
{
public:
    Transpose_M_Block(const std::string& name);
    ~Transpose_M_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

private:
    std::unique_ptr<Transpose_M> m_Transpose_M;
};
RegAlgo(Transpose_M_Block);
#endif // TRANSPOSE_M_BLOCK_H
