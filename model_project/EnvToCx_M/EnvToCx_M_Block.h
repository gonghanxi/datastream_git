#ifndef ENVTOCX_M_BLOCK_H
#define ENVTOCX_M_BLOCK_H
#include "EnvToCx_M.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API EnvToCx_M_Block : public Block
{
public:
    EnvToCx_M_Block(const std::string& name);
    ~EnvToCx_M_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;


private:
    std::unique_ptr<EnvToCx_M> m_env;
};

RegAlgo(EnvToCx_M_Block);

#endif // ENVTOCX_M_BLOCK_H
