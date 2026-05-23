#ifndef TRANSPOSE_BLOCK_H
#define TRANSPOSE_BLOCK_H
#include "Block.h"
#include "Transpose.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Transpose_Block : public Block
{
public:
    Transpose_Block(const std::string& name);
    ~Transpose_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    std::unique_ptr<Transpose> m_Transpose;

    int SamplesInRow;
    int NumberOfRows;
};
RegAlgo(Transpose_Block);

#endif // TRANSPOSE_BLOCK_H
