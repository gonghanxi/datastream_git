#ifndef SUBMXCX_M_BLOCK_H
#define SUBMXCX_M_BLOCK_H

#include "Block.h"
#include "SubMxCx_M.h"

#include <complex>
#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SubMxCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    SubMxCx_M_Block(const std::string& name);
    ~SubMxCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    std::unique_ptr<SubMxCx_M> m_SubMxCx_M;

    int m_StartRow;
    int m_StartCol;
    int m_NumRows;
    int m_NumCols;
};

RegAlgo(SubMxCx_M_Block);

#endif // SUBMXCX_M_BLOCK_H
