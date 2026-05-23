#ifndef ENVTOCX_BLOCK_H
#define ENVTOCX_BLOCK_H
#include "Block.h"
#include "EnvToCx.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API EnvToCx_Block : public SystemVueModelBuilder::Block
{
public:
    EnvToCx_Block(const std::string& name);
    ~EnvToCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;


private:
    void SetDefaultParameters();

    void UpdateCharacterizationFrequency();

    std::unique_ptr<EnvToCx> m_envtoCx;
};

RegAlgo(EnvToCx_Block);

#endif // ENVTOCX_BLOCK_H
