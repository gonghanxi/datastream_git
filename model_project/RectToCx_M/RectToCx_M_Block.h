#ifndef RECTTOCX_M_BLOCK_H
#define RECTTOCX_M_BLOCK_H

#include "Block.h"
#include "RectToCx_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RectToCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    RectToCx_M_Block(const std::string& name);
    ~RectToCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();

    std::unique_ptr<RectToCx_M> m_RectToCx_M;
};

RegAlgo(RectToCx_M_Block);

#endif // RECTTOCX_M_BLOCK_H
