#ifndef ADAPTLINQUANT_BLOCK_H
#define ADAPTLINQUANT_BLOCK_H

#include "Block.h"
#include "AdaptLinQuant.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AdaptLinQuant_Block : public SystemVueModelBuilder::Block
{
public:
    AdaptLinQuant_Block(const std::string& name);
    ~AdaptLinQuant_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<AdaptLinQuant> m_AdaptLinQuant;

    int m_Bits;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::vector<double> m_inStepBuffer;
    std::queue<double>  m_amplitudeQueue;
    std::queue<double>  m_outStepQueue;
    std::queue<int>     m_stepLevelQueue;
};

RegAlgo(AdaptLinQuant_Block);

#endif // ADAPTLINQUANT_BLOCK_H
