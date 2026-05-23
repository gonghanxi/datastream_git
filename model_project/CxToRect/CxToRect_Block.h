#ifndef CXTORECT_BLOCK_H
#define CXTORECT_BLOCK_H
#include "Block.h"
#include "CxToRect.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CxToRect_Block : public SystemVueModelBuilder::Block
{
public:
    CxToRect_Block(const std::string& name);
    ~CxToRect_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;



private:
    void SetDefaultParamters();

    std::unique_ptr<CxToRect> m_cxtoRect;
};

RegAlgo(CxToRect_Block);

#endif // CXTORECT_BLOCK_H
