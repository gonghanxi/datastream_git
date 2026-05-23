#ifndef TRANSPOSEENV_BLOCK_H
#define TRANSPOSEENV_BLOCK_H
#include "Block.h"
#include "TransposeEnv.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TransposeEnv_Block : public Block
{
public:
    TransposeEnv_Block(const std::string& name);
    ~TransposeEnv_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<TransposeEnv> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;
};
RegAlgo(TransposeEnv_Block);

#endif // TRANSPOSEENV_BLOCK_H
