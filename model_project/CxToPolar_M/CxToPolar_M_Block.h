#ifndef CXTOPOLAR_M_BLOCK_H
#define CXTOPOLAR_M_BLOCK_H

#include "Block.h"
#include "CxToPolar_M.h"

#include <complex>
#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CxToPolar_M_Block : public SystemVueModelBuilder::Block
{
public:
    CxToPolar_M_Block(const std::string& name);
    ~CxToPolar_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    std::unique_ptr<CxToPolar_M> m_CxToPolar_M;
};

RegAlgo(CxToPolar_M_Block);

#endif // CXTOPOLAR_M_BLOCK_H
