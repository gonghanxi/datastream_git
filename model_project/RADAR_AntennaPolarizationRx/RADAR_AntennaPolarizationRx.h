#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>

class SYSTEMVUEMODELBUILDER_API RADAR_AntennaPolarizationRx : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_AntennaPolarizationRx);

	RADAR_AntennaPolarizationRx();

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
	// Port 5: input_V, multiple envelope, optional
	// Port 6: input_H, multiple envelope, optional
	// =========================
	SystemVueModelBuilder::DoubleCircularBufferBus   TargetAzimuth;
	SystemVueModelBuilder::DoubleCircularBufferBus   TargetElevation;
	SystemVueModelBuilder::DoubleCircularBuffer      BeamAzimuth;
	SystemVueModelBuilder::DoubleCircularBuffer      BeamElevation;
	SystemVueModelBuilder::EnvelopeCircularBufferBus input_V;
	SystemVueModelBuilder::EnvelopeCircularBufferBus input_H;

	// =========================
	// 输出端口
	// Port 7: output_V, envelope, optional
	// Port 8: output_H, envelope, optional
	// =========================
	SystemVueModelBuilder::EnvelopeCircularBuffer output_V;
	SystemVueModelBuilder::EnvelopeCircularBuffer output_H;

	// =========================
	// 参数
	// =========================
	SelectedRadarWorkMode RadarWorkMode;

	SelectedElementPatternFileType ElementPatternFileType;

	double* ElementPatternFileScaleFactor;
	int     ElementPatternFileScaleFactor_Size;

	SelectedUserDefinedAntennaPattern UserDefinedAntennaPattern;

	// 文件名参数，SystemVue 中用 SetParamAsFile()
	char* RxAntennaPatternFileName1;

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
		double azDeg;
		double elDeg;

		std::complex<double> GHH;
		std::complex<double> GHV;
		std::complex<double> GVH;
		std::complex<double> GVV;
	};

	// =========================
	// 端口连接判断
	// 注意：这些函数不要加 const。
	// SystemVue 的 CircularBuffer / Bus operator[] / IsConnected()
	// 在 VS2017 下通常不是 const 成员。
	// =========================
	bool hasBeamAzimuthPort();
	bool hasBeamElevationPort();

	bool hasTargetAzimuthPort(int ch);
	bool hasTargetElevationPort(int ch);

	bool hasInputHPort(int ch);
	bool hasInputVPort(int ch);

	int determineTargetCount();

	// =========================
	// 参数数组读取
	// =========================
	double getArrayValue(const double* data,
		int size,
		int index,
		double defaultValue) const;

	double getScaleValue(int index) const;

	// =========================
	// 波束扫描角
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
	// 方向图文件读取与查找
	// =========================
	bool loadPatternFile();
	void clearPattern();
	bool parseNumericLine(const char* line,
		std::vector<double>& nums) const;

	void lookupPolarizationMatrix(double relAzDeg,
		double relElDeg,
		std::complex<double>& GHH,
		std::complex<double>& GHV,
		std::complex<double>& GVH,
		std::complex<double>& GVV) const;

	// =========================
	// 工具函数
	// =========================
	static double deg2rad(double x);
	static double rad2deg(double x);
	static double normalizeRad(double x);
	static double wrap360(double x);
	static double angleDiffDeg(double a, double b);
	static std::complex<double> dbPhaseToComplex(double db,
		double phaseDeg);

private:
	std::vector<PatternPoint> patternTable_;
	bool patternLoaded_;
};
