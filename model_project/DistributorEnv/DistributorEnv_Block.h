#ifndef DISTRIBUTORENV_BLOCK_H
#define DISTRIBUTORENV_BLOCK_H
#include "DistributorEnv.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API DistributorEnv_Block : public Block
{
public:
    DistributorEnv_Block(const std::string& name);
    ~DistributorEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<DistributorEnv> m_Distributor;

    int m_BlockSize;
    size_t m_iBlockSize;
};
RegAlgo(DistributorEnv_Block);

#endif // DISTRIBUTORENV_BLOCK_H
