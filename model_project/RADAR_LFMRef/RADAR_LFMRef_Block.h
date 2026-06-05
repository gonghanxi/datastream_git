#ifndef RADAR_LFMREF_BLOCK_H
#define RADAR_LFMREF_BLOCK_H

#include "Block.h"
#include "RADAR_LFMRef.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_LFMRef_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_LFMRef_Block(const std::string& name);
    ~RADAR_LFMRef_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    void GenerateFrame(std::vector<std::complex<double>>& outputData);

    std::unique_ptr<RADAR_LFMRef> m_algo;

    // ===== 参数 =====
    double m_Pulsewidth;
    double m_Bandwidth;
    double m_FM_Offset;
    double m_SampleRate;
    int    m_FFTSize;

    // ===== TimeDrivenRun =====
    std::queue<std::complex<double>> m_outputQueue;
};

RegAlgo(RADAR_LFMRef_Block);

#endif // RADAR_LFMREF_BLOCK_H
