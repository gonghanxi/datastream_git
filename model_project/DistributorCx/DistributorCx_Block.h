#ifndef DISTRIBUTORCX_BLOCK_H
#define DISTRIBUTORCX_BLOCK_H
#include "DistributorCx.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DistributorCx_Block : public Block
{
public:
    DistributorCx_Block(const std::string& name);
    ~DistributorCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<DistributorCx> m_Distributor;

    int m_BlockSize;
    size_t m_iBlockSize;
};
RegAlgo(DistributorCx_Block);

#endif // DISTRIBUTORCX_BLOCK_H
