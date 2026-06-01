#ifndef AVGSQRERR_M_BLOCK_H
#define AVGSQRERR_M_BLOCK_H

#include "Block.h"
#include "AvgSqrErr_M.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AvgSqrErr_M_Block : public SystemVueModelBuilder::Block
{
public:
    AvgSqrErr_M_Block(const std::string& name);
    ~AvgSqrErr_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<AvgSqrErr_M> m_AvgSqrErr_M;
    int m_NumInputsToAverage;

    // 时间驱动缓冲
    std::vector<SystemVueModelBuilder::Matrix<double>> m_input1Buffer;
    std::vector<SystemVueModelBuilder::Matrix<double>> m_input2Buffer;
    std::queue<double> m_outputQueue;
};

RegAlgo(AvgSqrErr_M_Block);

#endif // AVGSQRERR_M_BLOCK_H
