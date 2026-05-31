#ifndef DOWNSAMPLEVARPHASE_BLOCK_H
#define DOWNSAMPLEVARPHASE_BLOCK_H

#include "Block.h"
#include "DownSampleVarPhase.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API DownSampleVarPhase_Block : public SystemVueModelBuilder::Block
{
public:
    DownSampleVarPhase_Block(const std::string& name);
    ~DownSampleVarPhase_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<DownSampleVarPhase> m_DownSampleVarPhase;

    int m_Factor;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::vector<int>    m_phaseBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(DownSampleVarPhase_Block);

#endif // DOWNSAMPLEVARPHASE_BLOCK_H
