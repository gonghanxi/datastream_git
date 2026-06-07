#ifndef RADAR_ANTENNA_TX2_BLOCK_H
#define RADAR_ANTENNA_TX2_BLOCK_H

#include "Block.h"
#include "RADAR_Antenna_Tx2.h"

#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Antenna_Tx2_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Antenna_Tx2_Block(const std::string& name);
    ~RADAR_Antenna_Tx2_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    // enums
    using SelectedRadarWorkMode       = RADAR_Antenna_Tx2::SelectedRadarWorkMode;
    using SelectedPattern             = RADAR_Antenna_Tx2::SelectedPattern;
    using SelectedAntennaScanPattern  = RADAR_Antenna_Tx2::SelectedAntennaScanPattern;

    SelectedRadarWorkMode      ConvertStringToRadarWorkMode(const std::string& value);
    SelectedPattern            ConvertStringToPattern(const std::string& value);
    SelectedAntennaScanPattern ConvertStringToAntennaScanPattern(const std::string& value);

    // helpers
    static double deg2rad(double x);
    static double rad2deg(double x);
    static double normalizeRad(double x);
    static double wrap360(double x);
    static double sinc(double x);
    static double besselI0(double x);
    double getArrayValue(const double* data, int size, int index, double defaultValue) const;

    void   getBeamAngle(double timeNow, double& beamAzRad, double& beamElRad);
    double getCircularScanAzimuth(double timeNow) const;
    double getSectorScanAzimuth(double timeNow, bool bidirectional) const;
    void   getRasterScanAngle(double timeNow, bool bidirectional, double& azDeg, double& elDeg) const;

    double angularSeparation(double az1, double el1, double az2, double el2) const;
    double calcAntennaAmplitudeGain(double targetAzRad, double targetElRad, double beamAzRad, double beamElRad, double fcHz) const;
    double calcUserPatternGain(double targetAzRad, double targetElRad, double beamAzRad, double beamElRad) const;
    double calcAnalyticPatternFactor(double dAzRad, double dElRad, double lambda) const;

    std::unique_ptr<RADAR_Antenna_Tx2> m_ant;

    SelectedRadarWorkMode      m_RadarWorkMode;
    SelectedPattern            m_Pattern;
    double                     m_Factor1;
    double                     m_Factor2;
    double*                    m_AntennaPatternArray;
    int                        m_AntennaPatternArray_Size;
    double                     m_Sidelobe_Levels;
    int                        m_nBar;
    double                     m_AntennaHeight;
    double                     m_AntennaWidth;
    SelectedAntennaScanPattern m_AntennaScanPattern;
    double                     m_ScanRate;
    double                     m_ElevationAngle;
    double                     m_SectorScanStartAngle;
    double                     m_SectorScanEndAngle;
    double                     m_FlybackTime;
    double                     m_RetraceTime;
    int                        m_NumberOfRasterBars;
    double                     m_RasterBarWidth;
    double*                    m_TargetAzimuthAngle;
    int                        m_TargetAzimuthAngle_Size;
    double*                    m_TargetElevationAngle;
    int                        m_TargetElevationAngle_Size;
    double                     m_BeamAzimuthAngle;
    double                     m_BeamElevationAngle;
    double                     m_AntennaEfficiency;

    std::vector<double> primdata;

    // ===== 变步长缓冲 =====
    struct InputSnapshot {
        std::vector<double> targetAz;
        std::vector<double> targetEl;
        double beamAz;
        double beamEl;
        bool hasBeamAzPort;
        bool hasBeamElPort;
        EnvelopeSignal inputSignal;
        double fcHz;
    };
    std::vector<InputSnapshot> m_inputBuffer;

    struct OutputFrame {
        std::vector<EnvelopeSignal> outputSignals;
    };
    std::queue<OutputFrame> m_outputQueue;
};

RegAlgo(RADAR_Antenna_Tx2_Block);

#endif
