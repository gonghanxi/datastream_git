#ifndef TRANSPOSEINT_BLOCK_H
#define TRANSPOSEINT_BLOCK_H
#include "Block.h"
#include "TransposeInt.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TransposeInt_Block : public Block
{
public:
    TransposeInt_Block(const std::string& name);
    ~TransposeInt_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<TransposeInt> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;
};
RegAlgo(TransposeInt_Block);

#endif // TRANSPOSEINT_BLOCK_H
