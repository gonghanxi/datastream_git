#ifndef ABS_M_BLOCK_H
#define ABS_M_BLOCK_H

#include "Block.h"
#include "Abs_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Abs_M_Block : public SystemVueModelBuilder::Block
{
public:
    Abs_M_Block(const std::string& name);
    ~Abs_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();

    std::unique_ptr<Abs_M> m_Abs_M;
};

RegAlgo(Abs_M_Block);

#endif // ABS_M_BLOCK_H
