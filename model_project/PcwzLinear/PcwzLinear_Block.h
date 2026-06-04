#ifndef PCWZLINEAR_BLOCK_H
#define PCWZLINEAR_BLOCK_H

#include "Block.h"
#include "PcwzLinear.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PcwzLinear_Block : public SystemVueModelBuilder::Block
{
public:
    PcwzLinear_Block(const std::string& name);
    ~PcwzLinear_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<PcwzLinear> m_PcwzLinear;

    SystemVueModelBuilder::Matrix<std::complex<double>> m_Breakpoints;
    int m_numBreakpoints;
    std::vector<double> m_slope;
    std::vector<double> m_intercept;
    std::vector<double> m_breakpointsX;
    std::vector<double> m_breakpointsY;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(PcwzLinear_Block);

#endif // PCWZLINEAR_BLOCK_H
