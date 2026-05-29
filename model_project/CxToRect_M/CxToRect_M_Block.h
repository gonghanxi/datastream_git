#ifndef CXTORECT_M_BLOCK_H
#define CXTORECT_M_BLOCK_H

#include "Block.h"
#include "CxToRect_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CxToRect_M_Block : public SystemVueModelBuilder::Block
{
public:
    CxToRect_M_Block(const std::string& name);
    ~CxToRect_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();

    std::unique_ptr<CxToRect_M> m_CxToRect_M;
};

RegAlgo(CxToRect_M_Block);

#endif // CXTORECT_M_BLOCK_H
