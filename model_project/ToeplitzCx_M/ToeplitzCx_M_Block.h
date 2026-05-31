#ifndef TOEPLITZCX_M_BLOCK_H
#define TOEPLITZCX_M_BLOCK_H

#include "Block.h"
#include "ToeplitzCx_M.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ToeplitzCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    ToeplitzCx_M_Block(const std::string& name);
    ~ToeplitzCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<ToeplitzCx_M> m_ToeplitzCx_M;

    int m_NumRows;
    int m_NumCols;

    // 时间驱动缓冲
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DComplexMatrix> m_outputQueue;
};

RegAlgo(ToeplitzCx_M_Block);

#endif // TOEPLITZCX_M_BLOCK_H
