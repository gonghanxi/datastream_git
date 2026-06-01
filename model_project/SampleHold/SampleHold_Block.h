#ifndef SAMPLEHOLD_BLOCK_H
#define SAMPLEHOLD_BLOCK_H

#include "Block.h"
#include "SampleHold.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SampleHold_Block : public SystemVueModelBuilder::Block
{
public:
    SampleHold_Block(const std::string& name);
    ~SampleHold_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<SampleHold> m_SampleHold;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::vector<int>    m_clockBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(SampleHold_Block);

#endif // SAMPLEHOLD_BLOCK_H
