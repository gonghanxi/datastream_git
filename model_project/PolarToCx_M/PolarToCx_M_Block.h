#ifndef POLARTOCX_M_BLOCK_H
#define POLARTOCX_M_BLOCK_H

#include "Block.h"
#include "PolarToCx_M.h"

#include <complex>
#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PolarToCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    PolarToCx_M_Block(const std::string& name);
    ~PolarToCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    std::unique_ptr<PolarToCx_M> m_PolarToCx_M;
};

RegAlgo(PolarToCx_M_Block);

#endif // POLARTOCX_M_BLOCK_H
