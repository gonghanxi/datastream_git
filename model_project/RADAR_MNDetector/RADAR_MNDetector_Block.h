#ifndef RADAR_MNDETECTOR_BLOCK_H
#define RADAR_MNDETECTOR_BLOCK_H

#include "Block.h"
#include "RADAR_MNDetector.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MNDetector_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MNDetector_Block(const std::string& name);
    ~RADAR_MNDetector_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool validateAndPrepare();
    bool DataStreamRun();
    bool TimeDrivenRun();

    static int calcSamplesPerPRI(double pri, double sampleRate);

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_MNDetector> m_algo;

    // ---- parameters ----
    int    m_M;
    int    m_N;
    double m_PRI;
    double m_SampleRate;

    // ---- derived / cached ----
    int m_samplesPerPRI;
    int m_inputRate;
    int m_outputRate;

    // ---- TimeDrivenRun buffers ----
    std::vector<int> m_inputBuffer;
    std::queue<int>  m_outputQueue;
};

RegAlgo(RADAR_MNDetector_Block);

#endif // RADAR_MNDETECTOR_BLOCK_H
