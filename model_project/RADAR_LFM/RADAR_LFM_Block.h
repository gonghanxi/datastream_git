#ifndef RADAR_LFM_BLOCK_H
#define RADAR_LFM_BLOCK_H

#include "RADAR_LFM.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_LFM_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_LFM_Block(const std::string& name);
    ~RADAR_LFM_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double sampleRate = 1e6,
                       SystemVueModelBuilder::Matrix<double> pulsewidth = SystemVueModelBuilder::Matrix<double>(1,1),
                       SystemVueModelBuilder::Matrix<double> pri = SystemVueModelBuilder::Matrix<double>(1,1),
                       SystemVueModelBuilder::Matrix<int> pri_combination = SystemVueModelBuilder::Matrix<int>(1,1),
                       SystemVueModelBuilder::Matrix<double> bandwidth = SystemVueModelBuilder::Matrix<double>(1,1),
                       SystemVueModelBuilder::Matrix<double> fm_offset = SystemVueModelBuilder::Matrix<double>(1,1));

    int GetGeneratedSampleCount() const;
private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_LFM> m_radar_lfm;

    SystemVueModelBuilder::Matrix<double> m_Pulsewidth;
    SystemVueModelBuilder::Matrix<double> m_PRI;
    SystemVueModelBuilder::Matrix<int> m_PRI_Combination;
    SystemVueModelBuilder::Matrix<double> m_Bandwidth;
    SystemVueModelBuilder::Matrix<double> m_FM_Offset;
    double m_SampleRate;

    // 信号计数器
    int m_counter;
};

RegAlgo(RADAR_LFM_Block);

#endif // RADAR_LFM_BLOCK_H
