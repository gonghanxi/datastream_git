#ifndef AVERAGECXWOFFSET_BLOCK_H
#define AVERAGECXWOFFSET_BLOCK_H

#include "Block.h"
#include "AverageCxWOffset.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AverageCxWOffset_Block : public SystemVueModelBuilder::Block
{
public:
    AverageCxWOffset_Block(const std::string& name);
    ~AverageCxWOffset_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<AverageCxWOffset> m_AverageCxWOffset;

    int m_NumInputsToAverage;

    // 算法状态
    int                      m_InitialZeros;
    std::complex<double>     m_CurrentSum;
    std::complex<double>     m_CurrentAverage;
    int                      m_BufferIndex;

    // 时间驱动缓冲
    std::vector<std::complex<double>> m_inputBuffer;
    std::vector<int>                  m_offsetBuffer;
    std::queue<std::complex<double>>  m_outputQueue;
};

RegAlgo(AverageCxWOffset_Block);

#endif // AVERAGECXWOFFSET_BLOCK_H
