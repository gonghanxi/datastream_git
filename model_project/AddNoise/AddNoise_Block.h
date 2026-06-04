#ifndef ADDNOISE_BLOCK_H
#define ADDNOISE_BLOCK_H

#include "Block.h"
#include "AddNoise.h"

#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AddNoise_Block : public SystemVueModelBuilder::Block
{
public:
    AddNoise_Block(const std::string& name);
    ~AddNoise_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<AddNoise> m_AddNoise;

    double m_Bandwidth;
    double m_NoiseFigure;
    double m_SystemNoiseTemperature;
    double m_RefR;

    SimuParameter simulator_param;

    // 时间驱动缓冲
    std::vector<EnvelopeSignal> m_inputBuffer;
    std::queue<EnvelopeSignal>  m_outputQueue;
};

RegAlgo(AddNoise_Block);

#endif // ADDNOISE_BLOCK_H
