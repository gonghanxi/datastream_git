#ifndef SAMPLEMEAN_M_BLOCK_H
#define SAMPLEMEAN_M_BLOCK_H

#include "Block.h"
#include "SampleMean_M.h"

#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SampleMean_M_Block : public SystemVueModelBuilder::Block
{
public:
    SampleMean_M_Block(const std::string& name);
    ~SampleMean_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    std::unique_ptr<SampleMean_M> m_SampleMean_M;
};

RegAlgo(SampleMean_M_Block);

#endif // SAMPLEMEAN_M_BLOCK_H
