#ifndef SUBMX_M_BLOCK_H
#define SUBMX_M_BLOCK_H

#include "Block.h"
#include "SubMx_M.h"

#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SubMx_M_Block : public SystemVueModelBuilder::Block
{
public:
    SubMx_M_Block(const std::string& name);
    ~SubMx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    std::unique_ptr<SubMx_M> m_SubMx_M;

    int m_StartRow;
    int m_StartCol;
    int m_NumRows;
    int m_NumCols;
};

RegAlgo(SubMx_M_Block);

#endif // SUBMX_M_BLOCK_H
