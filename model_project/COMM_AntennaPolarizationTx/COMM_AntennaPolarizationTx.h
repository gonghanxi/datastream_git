#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <string>
#include <vector>

class COMM_AntennaPolarizationTx : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(COMM_AntennaPolarizationTx);

	COMM_AntennaPolarizationTx();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	// =====================================================================
	// 枚举参数
	// =====================================================================

	// 天线方向图数据来源。
	enum SelectedPatternDataMode
	{
		ParametricPattern = 0,
		ImportActualPattern = 1
	};

	// 参数化方向图类型。
	enum SelectedParametricPatternType
	{
		IsotropicPattern = 0,
		CosinePowerPattern = 1,
		GaussianPattern = 2,
		ThreeGPPPattern = 3
	};

	// 波束控制方式。
	enum SelectedBeamControlMode
	{
		FixedBeam = 0,
		BeamSweep = 1
	};

	// 导入文件类型。
	enum SelectedElementPatternFileType
	{
		EMPro = 0,
		HFSS = 1,
		CST = 2
	};

	// 导入方向图的维度。
	enum SelectedImportedPatternDimension
	{
		UserDefine2D = 0,
		UserDefine3D = 1
	};

	// 导入文件的增益处理方式。
	enum SelectedImportedGainMode
	{
		UseFileGain = 0,
		NormalizeFileToPeakGain = 1
	};

	// 极化方式。
	// PatternComponents：直接使用文件中的 theta/phi 两个复数分量。
	// 其余模式：使用配置参数生成归一化 Jones 矢量。
	enum SelectedPolarizationType
	{
		PatternComponents = 0,
		HorizontalPolarization = 1,
		VerticalPolarization = 2,
		LinearSlantPolarization = 3,
		RHCPPolarization = 4,
		LHCPPolarization = 5,
		UserDefinedJones = 6
	};

	// 波束扫描方式。
	enum SelectedBeamScanPattern
	{
		CircularScan = 0,
		BidirectionalSector = 1,
		UnidirectionalSector = 2,
		BidirectionalRaster = 3,
		UnidirectionalRaster = 4
	};

	// =====================================================================
	// 输入端口
	// Port 1: DirectionAzimuth, multiple real, optional, rad
	// Port 2: DirectionElevation, multiple real, optional, rad
	// Port 3: BeamAzimuth, real, optional, rad
	// Port 4: BeamElevation, real, optional, rad
	// Port 5: input, envelope, required
	// =====================================================================
	SystemVueModelBuilder::DoubleCircularBufferBus DirectionAzimuth;
	SystemVueModelBuilder::DoubleCircularBufferBus DirectionElevation;
	SystemVueModelBuilder::DoubleCircularBuffer BeamAzimuth;
	SystemVueModelBuilder::DoubleCircularBuffer BeamElevation;
	SystemVueModelBuilder::EnvelopeCircularBuffer input;

	// =====================================================================
	// 输出端口
	// Port 6: output_V, multiple envelope, required
	// Port 7: output_H, multiple envelope, required
	// =====================================================================
	SystemVueModelBuilder::EnvelopeCircularBufferBus output_V;
	SystemVueModelBuilder::EnvelopeCircularBufferBus output_H;

	// =====================================================================
	// 参数
	// =====================================================================

	SelectedPatternDataMode PatternDataMode;
	SelectedParametricPatternType ParametricPatternType;

	// 参数化方向图增益参数。
	double PeakGain_dBi;
	double AzimuthHPBW;
	double ElevationHPBW;
	double MaxAttenuation_dB;
	double VerticalSidelobeAttenuation_dB;

	// 极化参数。
	SelectedPolarizationType PolarizationType;
	double PolarizationTiltAngle;
	double XPD_dB;
	double CrossPolarPhaseAngle;

	double UserJonesHMagnitude;
	double UserJonesHPhase;
	double UserJonesVMagnitude;
	double UserJonesVPhase;

	// 实际方向图文件参数。
	SelectedElementPatternFileType ElementPatternFileType;

	double* ElementPatternFileScaleFactor;
	int ElementPatternFileScaleFactor_Size;

	SelectedImportedPatternDimension ImportedPatternDimension;
	SelectedImportedGainMode ImportedGainMode;
	char* TxAntennaPatternFileName1;

	// 波束控制参数。
	SelectedBeamControlMode BeamControlMode;
	SelectedBeamScanPattern BeamScanPattern;

	double ScanRate;
	double ElevationAngle;
	double SectorScanStartAngle;
	double SectorScanEndAngle;
	double FlybackTime;
	int NumberOfRasterBars;
	double RasterBarWidth;

	// 未连接 DirectionAzimuth/DirectionElevation 端口时使用的默认方向。
	double* DirectionAzimuthAngle;
	int DirectionAzimuthAngle_Size;

	double* DirectionElevationAngle;
	int DirectionElevationAngle_Size;

	// 未连接 BeamAzimuth/BeamElevation 端口时使用的固定波束方向。
	double BeamAzimuthAngle;
	double BeamElevationAngle;

private:
	struct PatternPoint
	{
		double thetaDeg;
		double phiDeg;

		std::complex<double> Gtheta; // theta 极化，对应 output_V
		std::complex<double> Gphi;   // phi 极化，对应 output_H
	};

	struct PatternFileOptions
	{
		bool useMagPhase;
		bool magnitudeInDb;
		bool directionInDegrees;
		bool phaseInDegrees;

		double phiMin;
		double phiMax;
		double phiInc;
		double thetaMin;
		double thetaMax;
		double thetaInc;

		PatternFileOptions() :
			useMagPhase(true),
			magnitudeInDb(true),
			directionInDegrees(true),
			phaseInDegrees(true),
			phiMin(0.0),
			phiMax(360.0),
			phiInc(1.0),
			thetaMin(0.0),
			thetaMax(180.0),
			thetaInc(1.0)
		{
		}
	};

private:
	// =====================================================================
	// 端口连接判断
	// 注意：不要加 const，SystemVue 的 IsConnected/operator[] 往往不是 const 成员。
	// =====================================================================
	bool hasBeamAzimuthPort();
	bool hasBeamElevationPort();
	bool hasDirectionAzimuthPort(int ch);
	bool hasDirectionElevationPort(int ch);

	int determineDirectionCount();

	// =====================================================================
	// 参数数组
	// =====================================================================
	double getArrayValue(const double* data,
		int size,
		int index,
		double defaultValue) const;

	double getScaleValue(int index) const;

	// =====================================================================
	// Beam 角度
	// =====================================================================
	void getBeamAngle(double timeNow,
		double& beamAzRad,
		double& beamElRad);

	double getCircularScanAzimuth(double timeNow) const;

	double getSectorScanAzimuth(double timeNow,
		bool bidirectional) const;

	void getRasterScanAngle(double timeNow,
		bool bidirectional,
		double& azDeg,
		double& elDeg) const;

	// =====================================================================
	// 参数化方向图和极化
	// =====================================================================
	bool validateConfiguration() const;

	double calculateParametricAttenuationDb(double relAzRad,
		double relElRad) const;

	double calculateCosineExponent(double halfPowerBeamwidthDeg) const;

	std::complex<double> calculateParametricScalarGain(double relAzRad,
		double relElRad) const;

	void buildPolarizationJones(std::complex<double>& jonesV,
		std::complex<double>& jonesH) const;

	void applyConfiguredPolarization(const std::complex<double>& scalarGain,
		std::complex<double>& Gtheta,
		std::complex<double>& Gphi) const;

	std::complex<double> combinePatternComponentsToScalar(
		const std::complex<double>& Gtheta,
		const std::complex<double>& Gphi) const;

	// =====================================================================
	// 实际方向图文件
	// =====================================================================
	bool loadPatternFile();
	void clearPattern();

	bool parseNumericLine(const char* line,
		std::vector<double>& nums) const;

	bool parseParameterLine(const std::string& line);

	void lookupImportedPolarizationGain(double thetaDeg,
		double phiDeg,
		std::complex<double>& Gtheta,
		std::complex<double>& Gphi) const;

	void azelToPatternThetaPhi(double relAzRad,
		double relElRad,
		double& thetaDeg,
		double& phiDeg) const;

	// =====================================================================
	// 工具函数
	// =====================================================================
	static std::string trim(const std::string& s);
	static std::string lowerString(const std::string& s);

	static double deg2rad(double x);
	static double rad2deg(double x);
	static double normalizeRad(double x);
	static double wrap360(double x);
	static double clamp(double x, double lo, double hi);
	static double angleDiffDeg(double a, double b);
	static double linearAmplitudeFromDb(double gainDb);

	static std::complex<double> magPhaseToComplex(double magnitude,
		double phase,
		bool magnitudeInDb,
		bool phaseInDegrees);

	static void normalizeJones(std::complex<double>& jonesV,
		std::complex<double>& jonesH);

private:
	std::vector<PatternPoint> patternTable_;
	PatternFileOptions patternOpt_;
	bool patternLoaded_;
	double patternPeakAmplitude_;
};
