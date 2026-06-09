#ifndef POLARTOCX_M_BLOCK_H
#define POLARTOCX_M_BLOCK_H

#include "Block.h"
#include "PolarToCx_M.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PolarToCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    PolarToCx_M_Block(const std::string& name);
    ~PolarToCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<PolarToCx_M> m_PolarToCx_M;

    // ===== TimeDrivenRun 输入缓冲(vector) + 输出队列 =====
    std::vector<SystemVueModelBuilder::DoubleMatrix>                 m_magBuffer;
    std::vector<SystemVueModelBuilder::DoubleMatrix>                 m_phaseBuffer;
    std::queue<SystemVueModelBuilder::Matrix<std::complex<double>>>  m_outputQueue;
};

RegAlgo(PolarToCx_M_Block);

#endif // POLARTOCX_M_BLOCK_H
