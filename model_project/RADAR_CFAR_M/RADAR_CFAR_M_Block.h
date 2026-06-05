#ifndef RADAR_CFAR_M_BLOCK_H
#define RADAR_CFAR_M_BLOCK_H

#include "Block.h"
#include "RADAR_CFAR_M.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CFAR_M_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_CFAR_M_Block(const std::string& name);
    ~RADAR_CFAR_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool ValidateParameters();
    void UpdateThresholdFactor();

    bool DataStreamRun();
    bool TimeDrivenRun();

    void processMatrix(
        const SystemVueModelBuilder::DoubleMatrix& inMat,
        SystemVueModelBuilder::DoubleMatrix& outMat,
        SystemVueModelBuilder::DoubleMatrix& thMat,
        int nRows, int nCols);

    RADAR_CFAR_M::SelectedCFARType      ConvertStringToCFARType(const std::string& value);
    RADAR_CFAR_M::SelectedCFARDimension ConvertStringToCFARDimension(const std::string& value);
    RADAR_CFAR_M::SelectedDetectorType  ConvertStringToDetectorType(const std::string& value);
    RADAR_CFAR_M::SelectedThresholdType ConvertStringToThresholdType(const std::string& value);

    std::unique_ptr<RADAR_CFAR_M> m_algo;

    RADAR_CFAR_M::SelectedCFARType      m_CFAR_Type;
    RADAR_CFAR_M::SelectedCFARDimension m_CFAR_Dimension;
    int m_CellSize;
    int m_ReferenceCell;
    int m_GuardCell;
    RADAR_CFAR_M::SelectedDetectorType  m_Detector_Type;
    RADAR_CFAR_M::SelectedThresholdType m_Threshold;
    double m_Pf;
    double m_Alpha;
    double m_Beta;
    double m_ThresholdFactor;

    // ClutterMap 跨帧状态
    bool m_clutterMapInitialized;
    SystemVueModelBuilder::Matrix<double> m_clutterMap;

    // ========== 时间驱动缓冲队列 ==========
    std::vector<SystemVueModelBuilder::DoubleMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DoubleMatrix> m_outputQueue;
    std::queue<SystemVueModelBuilder::DoubleMatrix> m_thresholdQueue;
    int m_inputCount;
    int m_outputCount;
};

RegAlgo(RADAR_CFAR_M_Block);

#endif // RADAR_CFAR_M_BLOCK_H
