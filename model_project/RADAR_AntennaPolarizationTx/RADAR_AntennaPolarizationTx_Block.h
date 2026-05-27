#ifndef RADAR_ANTENNAPOLARIZATIONTX_BLOCK_H
#define RADAR_ANTENNAPOLARIZATIONTX_BLOCK_H

#include "Block.h"
#include "RADAR_AntennaPolarizationTx.h"

#include <complex>
#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_AntennaPolarizationTx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_AntennaPolarizationTx_Block(const std::string& name);
    ~RADAR_AntennaPolarizationTx_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    // ---- enum aliases ----
    using SelectedRadarWorkMode             = RADAR_AntennaPolarizationTx::SelectedRadarWorkMode;
    using SelectedElementPatternFileType    = RADAR_AntennaPolarizationTx::SelectedElementPatternFileType;
    using SelectedUserDefinedAntennaPattern = RADAR_AntennaPolarizationTx::SelectedUserDefinedAntennaPattern;
    using SelectedAntennaScanPattern        = RADAR_AntennaPolarizationTx::SelectedAntennaScanPattern;

    SelectedRadarWorkMode             ConvertStringToRadarWorkMode(const std::string& value);
    SelectedElementPatternFileType    ConvertStringToElementPatternFileType(const std::string& value);
    SelectedUserDefinedAntennaPattern ConvertStringToUserDefinedAntennaPattern(const std::string& value);
    SelectedAntennaScanPattern        ConvertStringToAntennaScanPattern(const std::string& value);

    // ---- helpers ----
    static double deg2rad(double x);
    static double rad2deg(double x);
    static double normalizeRad(double x);
    static double wrapTo360(double x);
    static double clampValue(double x, double lo, double hi);
    static double angleDiffDeg(double a, double b);
    static std::complex<double> magPhaseToComplex(double magnitude, double phase,
                                                   bool magnitudeInDb, bool phaseInDegrees);

    double getArrayValue(const double* data, int size, int index, double defaultValue) const;
    double getScaleValue(int index) const;
    void   loadPatternFile();

    void   getBeamAngle(double timeNow, double& beamAzRad, double& beamElRad);
    double getCircularScanAzimuth(double timeNow) const;
    double getSectorScanAzimuth(double timeNow, bool bidirectional) const;
    void   getRasterScanAngle(double timeNow, bool bidirectional, double& azDeg, double& elDeg) const;

    void azelToPatternThetaPhi(double relAzRad, double relElRad,
                               double& thetaDeg, double& phiDeg) const;
    void lookupPolarizationGain(double thetaDeg, double phiDeg,
                                std::complex<double>& Gtheta,
                                std::complex<double>& Gphi) const;
    bool parseParameterLine(const std::string& line);
    bool parseNumericLine(const char* line, std::vector<double>& nums) const;

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_AntennaPolarizationTx> m_algo;

    // ---- parameters ----
    SelectedRadarWorkMode             m_RadarWorkMode;
    SelectedElementPatternFileType    m_ElementPatternFileType;
    double*                           m_ElementPatternFileScaleFactor;
    int                               m_ElementPatternFileScaleFactor_Size;
    std::vector<double>               m_ElementPatternFileScaleFactor_data;
    SelectedUserDefinedAntennaPattern m_UserDefinedAntennaPattern;
    std::string                       m_TxAntennaPatternFileName1;
    SelectedAntennaScanPattern        m_AntennaScanPattern;
    double                            m_ScanRate;
    double                            m_ElevationAngle;
    double                            m_SectorScanStartAngle;
    double                            m_SectorScanEndAngle;
    double                            m_FlybackTime;
    int                               m_NumberOfRasterBars;
    double                            m_RasterBarWidth;
    double*                           m_TargetAzimuthAngle;
    int                               m_TargetAzimuthAngle_Size;
    std::vector<double>               m_TargetAzimuthAngle_data;
    double*                           m_TargetElevationAngle;
    int                               m_TargetElevationAngle_Size;
    std::vector<double>               m_TargetElevationAngle_data;
    double                            m_BeamAzimuthAngle;
    double                            m_BeamElevationAngle;

    // ---- pattern table (Tx 使用 theta/phi 坐标系) ----
    struct PatternPoint {
        double thetaDeg;
        double phiDeg;
        std::complex<double> Gtheta;  // theta 分量 -> output_V
        std::complex<double> Gphi;    // phi   分量 -> output_H
    };

    struct PatternFileOptions {
        bool   useMagPhase;
        bool   magnitudeInDb;
        bool   directionInDegrees;
        bool   phaseInDegrees;
        double phiMin, phiMax, phiInc;
        double thetaMin, thetaMax, thetaInc;

        PatternFileOptions()
            : useMagPhase(true), magnitudeInDb(true)
            , directionInDegrees(true), phaseInDegrees(true)
            , phiMin(0.0), phiMax(360.0), phiInc(1.0)
            , thetaMin(0.0), thetaMax(180.0), thetaInc(1.0)
        {}
    };

    std::vector<PatternPoint> m_patternTable;
    PatternFileOptions        m_patternOpt;
    bool                      m_patternLoaded;
};

RegAlgo(RADAR_AntennaPolarizationTx_Block);

#endif // RADAR_ANTENNAPOLARIZATIONTX_BLOCK_H
