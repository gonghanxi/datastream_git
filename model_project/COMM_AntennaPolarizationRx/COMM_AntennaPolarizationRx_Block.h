#ifndef COMM_ANTENNAPOLARIZATIONRX_BLOCK_H
#define COMM_ANTENNAPOLARIZATIONRX_BLOCK_H

#include "COMM_AntennaPolarizationRx.h"
#include "Block.h"
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API COMM_AntennaPolarizationRx_Block : public SystemVueModelBuilder::Block
{
public:
    COMM_AntennaPolarizationRx_Block(const std::string& name);
    ~COMM_AntennaPolarizationRx_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 原算法实例指针（仅用于端口注册和参数设置）
    std::unique_ptr<COMM_AntennaPolarizationRx> m_algo;

    // ========== 参数存储 ==========
    int m_PatternDataMode;
    int m_ParametricPatternType;

    double m_PeakGain_dBi;
    double m_AzimuthHPBW;
    double m_ElevationHPBW;
    double m_MaxAttenuation_dB;
    double m_VerticalSidelobeAttenuation_dB;

    int m_PolarizationType;
    double m_PolarizationTiltAngle;
    double m_XPD_dB;
    double m_CrossPolarPhaseAngle;

    double m_UserJonesHMagnitude;
    double m_UserJonesHPhase;
    double m_UserJonesVMagnitude;
    double m_UserJonesVPhase;

    int m_ElementPatternFileType;
    std::vector<double> m_ElementPatternFileScaleFactor;
    int m_ImportedPatternDimension;
    int m_ImportedGainMode;
    std::string m_RxAntennaPatternFileName;

    int m_BeamControlMode;
    int m_BeamScanPattern;

    double m_ScanRate;
    double m_ElevationAngle;
    double m_SectorScanStartAngle;
    double m_SectorScanEndAngle;
    double m_FlybackTime;
    int m_NumberOfRasterBars;
    double m_RasterBarWidth;

    std::vector<double> m_DirectionAzimuthAngle;
    std::vector<double> m_DirectionElevationAngle;
    double m_BeamAzimuthAngle;
    double m_BeamElevationAngle;

    // ========== 运行时状态 ==========
    int m_directionCount;

    // ========== 时间驱动缓冲队列 ==========
    struct OutputFrame {
        std::complex<double> summedV;
        std::complex<double> summedH;
    };
    std::queue<OutputFrame> m_outputQueue;

    // ========== 方向图数据（从原算法移植） ==========
    struct PatternPoint
    {
        double thetaDeg;
        double phiDeg;
        std::complex<double> Gtheta;
        std::complex<double> Gphi;
    };

    struct PatternFileOptions
    {
        bool useMagPhase;
        bool magnitudeInDb;
        bool directionInDegrees;
        bool phaseInDegrees;
        double phiMin, phiMax, phiInc;
        double thetaMin, thetaMax, thetaInc;

        PatternFileOptions() :
            useMagPhase(true), magnitudeInDb(true),
            directionInDegrees(true), phaseInDegrees(true),
            phiMin(0.0), phiMax(360.0), phiInc(1.0),
            thetaMin(0.0), thetaMax(180.0), thetaInc(1.0) {}
    };

    std::vector<PatternPoint> m_patternTable;
    PatternFileOptions m_patternOpt;
    bool m_patternLoaded;
    double m_patternPeakAmplitude;

    // ========== 算法函数（从原算法移植到Block内部实现） ==========
    bool validateConfiguration() const;
    double getArrayValue(const double* data, int size, int index, double defaultValue) const;
    double getScaleValue(int index) const;

    // 波束角度计算
    double getCircularScanAzimuth(double timeNow) const;
    double getSectorScanAzimuth(double timeNow, bool bidirectional) const;
    void getRasterScanAngle(double timeNow, bool bidirectional, double& azDeg, double& elDeg) const;

    // 方向图计算
    double calculateParametricAttenuationDb(double relAzRad, double relElRad) const;
    double calculateCosineExponent(double halfPowerBeamwidthDeg) const;
    std::complex<double> calculateParametricScalarGain(double relAzRad, double relElRad) const;

    // 极化计算
    void buildPolarizationJones(std::complex<double>& jonesV, std::complex<double>& jonesH) const;
    void applyConfiguredPolarization(const std::complex<double>& scalarGain,
                                     std::complex<double>& Gtheta, std::complex<double>& Gphi) const;
    void applyReceivePolarization(const std::complex<double>& inputV,
                                  const std::complex<double>& inputH,
                                  const std::complex<double>& responseV,
                                  const std::complex<double>& responseH,
                                  std::complex<double>& outputV,
                                  std::complex<double>& outputH) const;
    std::complex<double> combinePatternComponentsToScalar(
        const std::complex<double>& Gtheta, const std::complex<double>& Gphi) const;

    // 实际方向图
    bool loadPatternFile();
    void clearPattern();
    bool parseNumericLine(const char* line, std::vector<double>& nums) const;
    bool parseParameterLine(const std::string& line);
    void lookupImportedPolarizationGain(double thetaDeg, double phiDeg,
                                        std::complex<double>& Gtheta, std::complex<double>& Gphi) const;
    void azelToPatternThetaPhi(double relAzRad, double relElRad, double& thetaDeg, double& phiDeg) const;

    // 工具函数
    static std::string trim(const std::string& s);
    static std::string lowerString(const std::string& s);
    static double deg2rad(double x);
    static double rad2deg(double x);
    static double normalizeRad(double x);
    static double wrap360(double x);
    static double clamp(double x, double lo, double hi);
    static double angleDiffDeg(double a, double b);
    static double linearAmplitudeFromDb(double gainDb);
    static std::complex<double> magPhaseToComplex(double magnitude, double phase,
                                                   bool magnitudeInDb, bool phaseInDegrees);
    static void normalizeJones(std::complex<double>& jonesV, std::complex<double>& jonesH);

    // 枚举解析
    static int ConvertStringToEnum(const std::string& value, int defaultValue);
};

RegAlgo(COMM_AntennaPolarizationRx_Block);

#endif // COMM_ANTENNAPOLARIZATIONRX_BLOCK_H
