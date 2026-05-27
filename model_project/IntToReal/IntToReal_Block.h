#ifndef INTTOREAL_BLOCK_H
#define INTTOREAL_BLOCK_H

#include "Block.h"
#include "IntToReal.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API IntToReal_Block : public SystemVueModelBuilder::Block
{
public:
    IntToReal_Block(const std::string& name);
    ~IntToReal_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetParameters();
    bool DataStreamRun();

    // ---- algorithm instance ----
    std::unique_ptr<IntToReal> m_algo;
};

RegAlgo(IntToReal_Block);

#endif // INTTOREAL_BLOCK_H
