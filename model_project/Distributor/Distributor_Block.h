#ifndef DISTRIBUTOR_BLOCK_H
#define DISTRIBUTOR_BLOCK_H
#include "Distributor.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Distributor_Block : public Block
{
public:
    Distributor_Block(const std::string& name);
    ~Distributor_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<Distributor> m_Distributor;

    int m_BlockSize;
    size_t m_iBlockSize;
};
RegAlgo(Distributor_Block);

#endif // DISTRIBUTOR_BLOCK_H
