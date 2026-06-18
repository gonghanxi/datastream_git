#ifndef ZEROCROSS_BLOCK_H
#define ZEROCROSS_BLOCK_H

#include "Block.h"
#include "ZeroCross.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ZeroCross_Block : public SystemVueModelBuilder::Block
{
public:
    ZeroCross_Block(const std::string& name);
    ~ZeroCross_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    std::unique_ptr<ZeroCross> m_zeroCross;
    double m_previousInput = 0.0;
};

RegAlgo(ZeroCross_Block);

#endif // ZEROCROSS_BLOCK_H
