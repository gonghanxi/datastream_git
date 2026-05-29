#ifndef CONJUGATE_M_BLOCK_H
#define CONJUGATE_M_BLOCK_H

#include "Block.h"
#include "Conjugate_M.h"

#include <memory>
#include <string>
#include <vector>
#include <complex>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Conjugate_M_Block : public SystemVueModelBuilder::Block
{
public:
    Conjugate_M_Block(const std::string& name);
    ~Conjugate_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();

    std::unique_ptr<Conjugate_M> m_Conjugate_M;
};

RegAlgo(Conjugate_M_Block);

#endif // CONJUGATE_M_BLOCK_H
