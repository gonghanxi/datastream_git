#ifndef RADAR_ANTENNAPOLARIZATIONRX_BLOCK_H
#define RADAR_ANTENNAPOLARIZATIONRX_BLOCK_H

#include "Block.h"
#include "RADAR_AntennaPolarizationRx.h"

#include <complex>
#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_AntennaPolarizationRx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_AntennaPolarizationRx_Block(const std::string& name);
    ~RADAR_AntennaPolarizationRx_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    // ---- enum aliases ----
    using SelectedRadarWorkMode          = RADAR_AntennaPolarizationRx::SelectedRadarWorkMode;
    using SelectedElementPatternFileType = RADAR_AntennaPolarizationRx::SelectedElementPatternFileType;
    using SelectedUserDefinedAntennaPattern = RADAR_AntennaPolarizationRx::SelectedUserDefinedAntennaPattern;
    using SelectedAntennaScanPattern     = RADAR_AntennaPolarizationRx::SelectedAntennaScanPattern;

    SelectedRadarWorkMode          ConvertStringToRadarWorkMode(const std::string& value);
    SelectedElementPatternFileType ConvertStringToElementPatternFileType(const std::string& value);
    SelectedUserDefinedAntennaPattern ConvertStringToUserDefinedAntennaPattern(const std::string& value);
    SelectedAntennaScanPattern     ConvertStringToAntennaScanPattern(const std::string& value);

    // ---- helpers ----
    static double deg2rad(double x);
    static double rad2deg(double x);
    static double normalizeRad(double x);
    static double wrapTo360(double x);
    static double angleDiffDeg(double a, double b);
    static std::complex<double> dbPhaseToComplex(double db, double phaseDeg);

    double getArrayValue(const double* data, int size, int index, double defaultValue) const;
    double getScaleValue(int index) const;
    void   loadPatternFile();

    void   getBeamAngle(double timeNow, double& beamAzRad, double& beamElRad);
    double getCircularScanAzimuth(double timeNow) const;
    double getSectorScanAzimuth(double timeNow, bool bidirectional) const;
    void   getRasterScanAngle(double timeNow, bool bidirectional, double& azDeg, double& elDeg) const;

    void lookupPolarizationMatrix(double relAzDeg, double relElDeg,
                                  std::complex<double>& GHH, std::complex<double>& GHV,
                                  std::complex<double>& GVH, std::complex<double>& GVV) const;

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_AntennaPolarizationRx> m_algo;

    // ---- parameters ----
    SelectedRadarWorkMode             m_RadarWorkMode;
    SelectedElementPatternFileType    m_ElementPatternFileType;
    double*                           m_ElementPatternFileScaleFactor;
    int                               m_ElementPatternFileScaleFactor_Size;
    std::vector<double>               m_ElementPatternFileScaleFactor_data;
    SelectedUserDefinedAntennaPattern m_UserDefinedAntennaPattern;
    std::string                       m_RxAntennaPatternFileName1;
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

    // ---- pattern table ----
    struct PatternPoint {
        double azDeg;
        double elDeg;
        std::complex<double> GHH;
        std::complex<double> GHV;
        std::complex<double> GVH;
        std::complex<double> GVV;
    };
    std::vector<PatternPoint> m_patternTable;
    bool                      m_patternLoaded;
};

RegAlgo(RADAR_AntennaPolarizationRx_Block);

#endif // RADAR_ANTENNAPOLARIZATIONRX_BLOCK_H
