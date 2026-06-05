#ifndef RADAR_DETECTOR_M_BLOCK_H
#define RADAR_DETECTOR_M_BLOCK_H

#include "Block.h"
#include "RADAR_Detector_M.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Detector_M_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Detector_M_Block(const std::string& name);
    ~RADAR_Detector_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    RADAR_Detector_M::SelectedDetectorType ConvertStringToDetectorType(const std::string& value);

    std::unique_ptr<RADAR_Detector_M> m_algo;

    // ===== 参数 =====
    RADAR_Detector_M::SelectedDetectorType m_Type;
    double m_Log_Coefb;
    double m_Log_Coefa;

    // ===== TimeDrivenRun =====
    std::vector<SystemVueModelBuilder::DComplexMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DoubleMatrix> m_outputQueue;
};

RegAlgo(RADAR_Detector_M_Block);

#endif // RADAR_DETECTOR_M_BLOCK_H
