#ifndef SUBCX_BLOCK_H
#define SUBCX_BLOCK_H

#include "Block.h"
#include "SubCx.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SubCx_Block : public SystemVueModelBuilder::Block
{
public:
    SubCx_Block(const std::string& name);
    ~SubCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();

    std::unique_ptr<SubCx> m_subCx;
};
RegAlgo(SubCx_Block);
#endif // SUBCX_BLOCK_H
