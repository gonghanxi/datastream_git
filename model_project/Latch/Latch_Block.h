#ifndef LATCH_BLOCK_H
#define LATCH_BLOCK_H

#include "Block.h"
#include "Latch.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Latch_Block : public SystemVueModelBuilder::Block
{
public:
    Latch_Block(const std::string& name);
    ~Latch_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<Latch> m_Latch;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::vector<int>    m_clockBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(Latch_Block);

#endif // LATCH_BLOCK_H
