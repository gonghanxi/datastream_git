#ifndef RADAR_DETECTOR_BLOCK_H
#define RADAR_DETECTOR_BLOCK_H

#include "Block.h"
#include "RADAR_Detector.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Detector_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Detector_Block(const std::string& name);
    ~RADAR_Detector_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    RADAR_Detector::SelectedDetectorType ConvertStringToDetectorType(const std::string& value);

    std::unique_ptr<RADAR_Detector> m_radarDetector;

    RADAR_Detector::SelectedDetectorType m_detectorType;
    double m_logCoefb;
    double m_logCoefa;
};

RegAlgo(RADAR_Detector_Block);

#endif // RADAR_DETECTOR_BLOCK_H
