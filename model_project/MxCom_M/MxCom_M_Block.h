#ifndef MXCOM_M_BLOCK_H
#define MXCOM_M_BLOCK_H

#include "Block.h"
#include "MxCom_M.h"

#include <memory>
#include <queue>
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
    bool DataStreamRun();
    bool TimeDrivenRun();

    int m_OutputNumRows;
    int m_OutputNumCols;
    int m_InputNumRows;
    int m_InputNumCols;
    int m_numSubMatrices;

    std::unique_ptr<MxCom_M> m_MxCom_M;

    // ===== TimeDrivenRun 输入缓冲 + 输出队列 =====
    std::vector<SystemVueModelBuilder::DoubleMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DoubleMatrix>  m_outputQueue;
};

RegAlgo(MxCom_M_Block);

#endif // MXCOM_M_BLOCK_H
