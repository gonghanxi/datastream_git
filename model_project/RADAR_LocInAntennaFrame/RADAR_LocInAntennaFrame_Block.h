#ifndef RADAR_LOCINANTENNAFRAME_BLOCK_H
#define RADAR_LOCINANTENNAFRAME_BLOCK_H

#include "Block.h"
#include "RADAR_LocInAntennaFrame.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_LocInAntennaFrame_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_LocInAntennaFrame_Block(const std::string& name);
    ~RADAR_LocInAntennaFrame_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static RADAR_LocInAntennaFrame::SelectedXYZFrameTypes      ConvertStringToXYZFrameType(const std::string& value);
    static RADAR_LocInAntennaFrame::SelectedAntennaPlaneTypes  ConvertStringToAntennaPlaneType(const std::string& value);
    static RADAR_LocInAntennaFrame::SelectedCoordinateTypes    ConvertStringToCoordinateType(const std::string& value);

    std::unique_ptr<RADAR_LocInAntennaFrame> m_algo;

    // ===== 参数 =====
    double                                       m_TimeStep;
    RADAR_LocInAntennaFrame::SelectedXYZFrameTypes     m_XYZFrameType;
    RADAR_LocInAntennaFrame::SelectedAntennaPlaneTypes m_AntennaPlaneType;
    RADAR_LocInAntennaFrame::SelectedCoordinateTypes   m_CoordinateType;

    // ===== 可选角度输入 =====
    double m_BodyYawAngle;
    double m_BodyPitchAngle;
    double m_BodyRollAngle;
    double m_AntYawAngle;
    double m_AntPitchAngle;
    double m_AntRollAngle;

    // ===== 算法状态 =====
    int m_TargetNum;

    // ===== TimeDrivenRun 逐点累积 =====
    std::vector<double> m_radarLocBuffer;
    std::vector<double> m_targetLocBuffer;
    std::queue<std::vector<double>> m_azimuthQueue;
    std::queue<std::vector<double>> m_elevationQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;
};

RegAlgo(RADAR_LocInAntennaFrame_Block);

#endif // RADAR_LOCINANTENNAFRAME_BLOCK_H
