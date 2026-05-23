#ifndef CXTOENV_M_BLOCK_H
#define CXTOENV_M_BLOCK_H
#include "Block.h"
#include "CxToEnv_M.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CxToEnv_M_Block : public Block
{
public:
    CxToEnv_M_Block(const std::string& name);
    ~CxToEnv_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<CxToEnv_M> m_cx;

    double Fc;
};
RegAlgo(CxToEnv_M_Block);
#endif // CXTOENV_M_BLOCK_H
