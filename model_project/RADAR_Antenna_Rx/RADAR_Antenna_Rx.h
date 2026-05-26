#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "CircularBuffer.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API RADAR_Antenna_Rx : public SystemVueModelBuilder::TimedDFModel
{
public:
	// ============================================================
	// Enums
	// ============================================================
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

public:
	DECLARE_MODEL_INTERFACE(RADAR_Antenna_Rx);

	RADAR_Antenna_Rx();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	// ============================================================
	// Ports
	// Port 1 : TargetAzimuth, multiple real, optional, radian
	// Port 2 : TargetElevation, multiple real, optional, radian
	// Port 3 : BeamAzimuth, real, optional, radian
	// Port 4 : BeamElevation, real, optional, radian
	// Port 5 : input, multiple envelope, required
	// Port 6 : output, envelope, required
	// ============================================================

	SystemVueModelBuilder::DoubleCircularBufferBus TargetAzimuth;
	SystemVueModelBuilder::DoubleCircularBufferBus TargetElevation;
	SystemVueModelBuilder::DoubleCircularBuffer    BeamAzimuth;
	SystemVueModelBuilder::DoubleCircularBuffer    BeamElevation;
	SystemVueModelBuilder::EnvelopeCircularBufferBus input;
	SystemVueModelBuilder::EnvelopeCircularBuffer    output;

	// ============================================================
	// Parameters
	// ============================================================

	SelectedRadarWorkMode RadarWorkMode;
	SelectedPattern       Pattern;

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
	int    NumberOfRasterBars;
	double RasterBarWidth;

	double* TargetAzimuthAngle;
	int     TargetAzimuthAngle_Size;

	double* TargetElevationAngle;
	int     TargetElevationAngle_Size;

	double BeamAzimuthAngle;
	double BeamElevationAngle;

private:
	static double deg2rad(double x);
	static double rad2deg(double x);
	static double wrapToPi(double x);
	static double wrapTo360(double x);
	static double clampValue(double v, double lo, double hi);
	static double sinc(double x);
	static double besselI0(double x);

private:
	bool hasBeamAzimuthPort();
	bool hasBeamElevationPort();
	bool hasTargetAzimuthPort(int ch);
	bool hasTargetElevationPort(int ch);

	double getArrayValue(const double* data,
		int size,
		int index,
		double defaultValue) const;

private:
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

private:
	double angularSeparation(double az1,
		double el1,
		double az2,
		double el2) const;

	double calcPatternGainDb(double targetAzRad,
		double targetElRad,
		double beamAzRad,
		double beamElRad,
		double fcHz) const;

	double calcUserPatternGainDb(double dAzRad,
		double dElRad) const;

	double calcAnalyticPatternGainDb(double dAzRad,
		double dElRad,
		double fcHz) const;

	double calcDistributionWeight(double uNorm) const;

	double calcApertureGainLinear(double fcHz) const;
};
