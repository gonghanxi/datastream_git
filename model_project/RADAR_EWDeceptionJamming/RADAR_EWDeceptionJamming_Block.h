#ifndef RADAR_EWDECEPTIONJAMMING_BLOCK_H
#define RADAR_EWDECEPTIONJAMMING_BLOCK_H

#include "Block.h"
#include "RADAR_EWDeceptionJamming.h"

#include <complex>
#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_EWDeceptionJamming_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_EWDeceptionJamming_Block(const std::string& name);
    ~RADAR_EWDeceptionJamming_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_EWDeceptionJamming> m_algo;

    // ---- parameters ----
    int                               m_SampleNum;
    double                            m_SampleRate;
    int                               m_FalseTargetNum;
    double                            m_MaxRange;
    double                            m_System_Loss;
    SystemVueModelBuilder::Matrix<double> m_FalseTargetRangeDelay;
    SystemVueModelBuilder::Matrix<double> m_FalseTargetDopplerOffset;
    SystemVueModelBuilder::Matrix<double> m_FalseTargetGain;

    // ---- internal state (Block 层自行维护) ----
    SystemVueModelBuilder::Matrix<std::complex<double>> m_FalseTargetDelayBuffer;
    int m_MaxSampleNum;
    int m_SampleIndex;
};

RegAlgo(RADAR_EWDeceptionJamming_Block);

#endif // RADAR_EWDECEPTIONJAMMING_BLOCK_H
