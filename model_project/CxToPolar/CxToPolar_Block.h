#pragma once

#include "CxToPolar.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CxToPolar_Block : public SystemVueModelBuilder::Block
{
public:
    CxToPolar_Block(const std::string& name);
    ~CxToPolar_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    std::unique_ptr<CxToPolar> m_cxToPolar;
};

RegAlgo(CxToPolar_Block);
