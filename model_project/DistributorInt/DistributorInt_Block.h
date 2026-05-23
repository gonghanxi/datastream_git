#ifndef DISTRIBUTORINT_BLOCK_H
#define DISTRIBUTORINT_BLOCK_H
#include "DistributorInt.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DistributorInt_Block : public Block
{
public:
    DistributorInt_Block(const std::string& name);
    ~DistributorInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<DistributorInt> m_Distributor;

    int m_BlockSize;
    size_t m_iBlockSize;
};
RegAlgo(DistributorInt_Block);

#endif // DISTRIBUTORINT_BLOCK_H
