#ifndef MXCOM_M_BLOCK_H
#define MXCOM_M_BLOCK_H

#include "Block.h"
#include "MxCom_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MxCom_M_Block : public SystemVueModelBuilder::Block
{
public:
    MxCom_M_Block(const std::string& name);
    ~MxCom_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    int m_OutputNumRows;
    int m_OutputNumCols;
    int m_InputNumRows;
    int m_InputNumCols;

    std::unique_ptr<MxCom_M> m_MxCom_M;
};

RegAlgo(MxCom_M_Block);

#endif // MXCOM_M_BLOCK_H
