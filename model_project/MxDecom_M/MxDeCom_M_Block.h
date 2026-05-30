#ifndef MXDECOM_M_BLOCK_H
#define MXDECOM_M_BLOCK_H

#include "Block.h"
#include "MxDecom_M.h"

#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MxDeCom_M_Block : public SystemVueModelBuilder::Block
{
public:
    MxDeCom_M_Block(const std::string& name);
    ~MxDeCom_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    int m_StartRow;
    int m_StartCol;
    int m_InputNumRows;
    int m_InputNumCols;
    int m_OutputNumRows;
    int m_OutputNumCols;

    std::unique_ptr<MxDecom_M> m_MxDecom_M;
};

RegAlgo(MxDeCom_M_Block);

#endif // MXDECOM_M_BLOCK_H
