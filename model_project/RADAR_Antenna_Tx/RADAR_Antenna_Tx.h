#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>

class SYSTEMVUEMODELBUILDER_API RADAR_Antenna_Tx : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_Antenna_Tx);

	RADAR_Antenna_Tx();

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

	enum SelectedPattern
	{
		UserDefinedPattern = 0,
		Uniform = 1,
		Cosine = 2,
		Parabolic = 3,
		Triangle = 4,
		Circular = 5,
		CosineSquaredPedestal = 6,
		Taylor = 7
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
	// Port 1/2: multiple real, optional
	// Port 3/4: real, optional
	// Port 5: envelope, required
	// =========================
	SystemVueModelBuilder::DoubleCircularBufferBus TargetAzimuth;
	SystemVueModelBuilder::DoubleCircularBufferBus TargetElevation;
	SystemVueModelBuilder::DoubleCircularBuffer    BeamAzimuth;
	SystemVueModelBuilder::DoubleCircularBuffer    BeamElevation;
	SystemVueModelBuilder::EnvelopeCircularBuffer  input;

	// =========================
	// 输出端口
	// Port 6: multiple envelope
	// =========================
	SystemVueModelBuilder::EnvelopeCircularBufferBus output;

	// =========================
	// 参数
	// =========================
	SelectedRadarWorkMode RadarWorkMode;
	SelectedPattern Pattern;

	double Factor1;
	double Factor2;

	double* AntennaPatternArray;
	int     AntennaPatternArray_Size;

	double Sidelobe_Levels;
	int    nBar;

	double AntennaHeight;
	double AntennaWidth;

	SelectedAntennaScanPattern AntennaScanPattern;

	double ScanRate;
	double ElevationAngle;
	double SectorScanStartAngle;
	double SectorScanEndAngle;
	double FlybackTime;
	double RetraceTime;
	int    NumberOfRasterBars;
	double RasterBarWidth;

	double* TargetAzimuthAngle;
	int     TargetAzimuthAngle_Size;

	double* TargetElevationAngle;
	int     TargetElevationAngle_Size;

	double BeamAzimuthAngle;
	double BeamElevationAngle;

private:
	// 注意：
	// 下面这些函数不能加 const。
	// 因为 SystemVue 的 CircularBuffer / CircularBufferBusT 的
	// IsConnected()、operator[] 不是 const 成员函数。
	int  determineTargetCount();

	bool hasBeamAzimuthPort();
	bool hasBeamElevationPort();
	bool hasTargetAzimuthPort(int ch);
	bool hasTargetElevationPort(int ch);

	double getArrayValue(const double* data,
		int size,
		int index,
		double defaultValue) const;

	void   getBeamAngle(double timeNow,
		double& beamAzRad,
		double& beamElRad);

	double getCircularScanAzimuth(double timeNow) const;
	double getSectorScanAzimuth(double timeNow, bool bidirectional) const;

	void   getRasterScanAngle(double timeNow,
		bool bidirectional,
		double& azDeg,
		double& elDeg) const;

	double calcAntennaAmplitudeGain(double targetAzRad,
		double targetElRad,
		double beamAzRad,
		double beamElRad,
		double fcHz) const;

	double calcUserPatternGain(double targetAzRad,
		double targetElRad,
		double beamAzRad,
		double beamElRad) const;

	double calcAnalyticPatternFactor(double dAzRad,
		double dElRad,
		double lambda) const;

	double angularSeparation(double az1,
		double el1,
		double az2,
		double el2) const;

	static double deg2rad(double x);
	static double rad2deg(double x);
	static double normalizeRad(double x);
	static double wrap360(double x);
	static double sinc(double x);
	static double besselI0(double x);
};
