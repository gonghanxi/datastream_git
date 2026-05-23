#ifndef RECTTOCX_BLOCK_H
#define RECTTOCX_BLOCK_H
#include "RectToCx.h"
#include "Block.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RectToCx_Block : public SystemVueModelBuilder::Block
{
public:
    RectToCx_Block(const std::string& name);
    ~RectToCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    int GetBatchSize() const override;
    int RunBatch(int maxCount) override;

private:

    std::unique_ptr<RectToCx> m_rectoCx;
    int m_batchSize = 10;

};

RegAlgo(RectToCx_Block);
#endif // RECTTOCX_BLOCK_H
