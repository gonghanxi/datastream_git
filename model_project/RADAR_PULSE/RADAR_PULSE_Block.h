#ifndef RADAR_PULSE_BLOCK_H
#define RADAR_PULSE_BLOCK_H

#include "Block.h"
#include "RADAR_PULSE.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API  RADAR_PULSE_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_PULSE_Block(const std::string& name);
    ~RADAR_PULSE_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double sampleRate = 1e6,
                            SystemVueModelBuilder::Matrix<double> pulsewidth = SystemVueModelBuilder::Matrix<double>(1,1),
                            SystemVueModelBuilder::Matrix<double> pri = SystemVueModelBuilder::Matrix<double>(1,1),
                            SystemVueModelBuilder::Matrix<int> pri_combination = SystemVueModelBuilder::Matrix<int>(1,1)
                            );

    int GetGeneratedSampleCount() const;
private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_PULSE> m_radarPULSE;
    SystemVueModelBuilder::Matrix<double> m_pulsewidth;
    SystemVueModelBuilder::Matrix<double> m_PRI;
    SystemVueModelBuilder::Matrix<int> m_PRI_Combination;
    double m_sampleRate;
    int m_counter;

};

RegAlgo(RADAR_PULSE_Block);

#endif // RADAR_PULSE_BLOCK_H
