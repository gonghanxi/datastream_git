#ifndef RADAR_NLFM_BLOCK_H
#define RADAR_NLFM_BLOCK_H

#include "Block.h"
#include "RADAR_NLFM.h"
#include "DataTypesAndParsers.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_NLFM_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_NLFM_Block(const std::string& name);
    ~RADAR_NLFM_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    RADAR_NLFM::NLF_Types ConvertStringToNLFType(const std::string& value);

    std::unique_ptr<RADAR_NLFM> m_radarNlfm;

    double m_pulsewidth;
    double m_pri;
    double m_bandwidth;
    double m_sampleRate;
    RADAR_NLFM::NLF_Types m_nlfType;
    SystemVueModelBuilder::Matrix<double> m_polyCoef;
};

RegAlgo(RADAR_NLFM_Block);

#endif // RADAR_NLFM_BLOCK_H
