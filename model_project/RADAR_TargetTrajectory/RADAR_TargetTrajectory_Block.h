#ifndef RADAR_TARGETTRAJECTORY_BLOCK_H
#define RADAR_TARGETTRAJECTORY_BLOCK_H
#include "RADAR_TargetTrajectory.h"
#include "Block.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_TargetTrajectory_Block : public Block
{
public:
    RADAR_TargetTrajectory_Block(const std::string& name);
    ~RADAR_TargetTrajectory_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    RADAR_TargetTrajectory::Coordinate_ModeEnum ConvertStringToCoordinate_ModeEnum(const std::string& value);
    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    std::unique_ptr<RADAR_TargetTrajectory> m_radar;

    RADAR_TargetTrajectory::Coordinate_ModeEnum Coordinate_Mode;

    double Range_Initial;
    double ElevationAngle;
    double AzimuthAngle;
    double Velocity_Initial;
    double Accelerate;
    double Jerk;

    double* Position_Initial_XYZ;
    int     Position_Initial_XYZSize;
    std::vector<double> Position_Initial_XYZ_data;

    double* Velocity_Initial_XYZ;
    int     Velocity_Initial_XYZSize;
    std::vector<double> Velocity_Initial_XYZ_data;

    double* Accelerate_XYZ;
    int     Accelerate_XYZSize;
    std::vector<double> Accelerate_XYZ_data;

    double* Jerk_XYZ;
    int     Jerk_XYZSize;
    std::vector<double> Jerk_XYZ_data;

    double TimeStep;
};
RegAlgo(RADAR_TargetTrajectory_Block);
#endif // RADAR_TARGETTRAJECTORY_BLOCK_H
