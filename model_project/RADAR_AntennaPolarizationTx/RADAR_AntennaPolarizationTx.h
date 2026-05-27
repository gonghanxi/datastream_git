#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>

class SYSTEMVUEMODELBUILDER_API RADAR_AntennaPolarizationTx : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_AntennaPolarizationTx);

	RADAR_AntennaPolarizationTx();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	// =========================
	// 枚举参数
	// =========================
	enum SelectedRadarWorkMode
	{
		Tracking = 0,
		Search = 1
	};

	enum SelectedElementPatternFileType
	{
		EMPro = 0,
		HFSS = 1,
		CST = 2
	};

	enum SelectedUserDefinedAntennaPattern
	{
		UserDefine2D = 0,
		UserDefine3D = 1
	};

	enum SelectedAntennaScanPattern
	{
		CircularScan = 0,
		BidirectionalSector = 1,
		UnidirectionalSector = 2,
		BidirectionalRaster = 3,
		UnidirectionalRaster = 4
	};

	// =========================
	// 输入端口
	// Port 1: TargetAzimuth, multiple real, optional, rad
	// Port 2: TargetElevation, multiple real, optional, rad
	// Port 3: BeamAzimuth, real, optional, rad
	// Port 4: BeamElevation, real, optional, rad
	// Port 5: input, envelope, required
	// =========================
	SystemVueModelBuilder::DoubleCircularBufferBus  TargetAzimuth;
	SystemVueModelBuilder::DoubleCircularBufferBus  TargetElevation;
	SystemVueModelBuilder::DoubleCircularBuffer     BeamAzimuth;
	SystemVueModelBuilder::DoubleCircularBuffer     BeamElevation;
	SystemVueModelBuilder::EnvelopeCircularBuffer   input;

	// =========================
	// 输出端口
	// Port 6: output_V, multiple envelope, required
	// Port 7: output_H, multiple envelope, required
	// =========================
	SystemVueModelBuilder::EnvelopeCircularBufferBus output_V;
	SystemVueModelBuilder::EnvelopeCircularBufferBus output_H;

	// =========================
	// 参数
	// =========================
	SelectedRadarWorkMode RadarWorkMode;

	SelectedElementPatternFileType ElementPatternFileType;

	double* ElementPatternFileScaleFactor;
	int     ElementPatternFileScaleFactor_Size;

	SelectedUserDefinedAntennaPattern UserDefinedAntennaPattern;

	char* TxAntennaPatternFileName1;

	SelectedAntennaScanPattern AntennaScanPattern;

	double ScanRate;
	double ElevationAngle;
	double SectorScanStartAngle;
	double SectorScanEndAngle;
	double FlybackTime;
	int    NumberOfRasterBars;
	double RasterBarWidth;

	double* TargetAzimuthAngle;
	int     TargetAzimuthAngle_Size;

	double* TargetElevationAngle;
	int     TargetElevationAngle_Size;

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

		PatternFileOptions()
			: useMagPhase(true)
			, magnitudeInDb(true)
			, directionInDegrees(true)
			, phaseInDegrees(true)
			, phiMin(0.0)
			, phiMax(360.0)
			, phiInc(1.0)
			, thetaMin(0.0)
			, thetaMax(180.0)
			, thetaInc(1.0)
		{
		}
	};

private:
	// =========================
	// 端口连接判断
	// 注意：不要加 const，SystemVue 的 IsConnected/operator[] 往往不是 const 成员。
	// =========================
	bool hasBeamAzimuthPort();
	bool hasBeamElevationPort();
	bool hasTargetAzimuthPort(int ch);
	bool hasTargetElevationPort(int ch);

	int determineTargetCount();

	// =========================
	// 参数数组
	// =========================
	double getArrayValue(const double* data,
		int size,
		int index,
		double defaultValue) const;

	double getScaleValue(int index) const;

	// =========================
	// Beam 角度
	// =========================
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

	// =========================
	// 方向图查表
	// =========================
	bool loadPatternFile();
	void clearPattern();

	bool parseNumericLine(const char* line,
		std::vector<double>& nums) const;

	bool parseParameterLine(const std::string& line);

	void lookupPolarizationGain(double thetaDeg,
		double phiDeg,
		std::complex<double>& Gtheta,
		std::complex<double>& Gphi) const;

	void azelToPatternThetaPhi(double relAzRad,
		double relElRad,
		double& thetaDeg,
		double& phiDeg) const;

	// =========================
	// 工具函数
	// =========================
	static std::string trim(const std::string& s);
	static std::string lowerString(const std::string& s);

	static double deg2rad(double x);
	static double rad2deg(double x);
	static double normalizeRad(double x);
	static double wrap360(double x);
	static double clamp(double x, double lo, double hi);
	static double angleDiffDeg(double a, double b);

	static std::complex<double> magPhaseToComplex(double magnitude,
		double phase,
		bool magnitudeInDb,
		bool phaseInDegrees);

private:
	std::vector<PatternPoint> patternTable_;
	PatternFileOptions        patternOpt_;
	bool                      patternLoaded_;
};
