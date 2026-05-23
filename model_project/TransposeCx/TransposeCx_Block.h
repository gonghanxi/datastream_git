#ifndef TRANSPOSECX_BLOCK_H
#define TRANSPOSECX_BLOCK_H
#include "Block.h"
#include "TransposeCx.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TransposeCx_Block : public Block
{
public:
    TransposeCx_Block(const std::string& name);
    ~TransposeCx_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<TransposeCx> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;
};
RegAlgo(TransposeCx_Block);

#endif // TRANSPOSECX_BLOCK_H
