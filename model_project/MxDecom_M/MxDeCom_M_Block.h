#ifndef MXDECOM_M_BLOCK_H
#define MXDECOM_M_BLOCK_H

#include "Block.h"
#include "MxDecom_M.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

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
    bool DataStreamRun();
    bool TimeDrivenRun();

    int m_StartRow;
    int m_StartCol;
    int m_InputNumRows;
    int m_InputNumCols;
    int m_OutputNumRows;
    int m_OutputNumCols;
    int m_numSubMatrices;

    std::unique_ptr<MxDecom_M> m_MxDecom_M;

    // ===== TimeDrivenRun 输入缓冲(vector) + 输出队列 =====
    std::vector<SystemVueModelBuilder::DoubleMatrix>  m_inputBuffer;
    std::queue<SystemVueModelBuilder::DoubleMatrix>   m_outputQueue;
};

RegAlgo(MxDeCom_M_Block);

#endif // MXDECOM_M_BLOCK_H
