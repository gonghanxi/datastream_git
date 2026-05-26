#pragma once

#include "ModelBuilder.h"

#include <complex>
#include <vector>

class SYSTEMVUEMODELBUILDER_API RADAR_ADBF : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_ADBF);

	RADAR_ADBF();

	virtual bool Setup();
	virtual bool Run();

    int getNumX() const;
    int getNumY() const;

	// =========================
	// Ports
	// =========================
	// Port 1: input, multiple complex, required
	SystemVueModelBuilder::DComplexCircularBufferBus input;

	// Port 2: el, real, optional, degree
	SystemVueModelBuilder::DoubleCircularBuffer el;

	// Port 3: az, real, optional, degree
	SystemVueModelBuilder::DoubleCircularBuffer az;

	// Port 4: weight, multiple complex, required
	SystemVueModelBuilder::DComplexCircularBufferBus weight;

	// =========================
	// Parameters
	// =========================
    std::vector<SystemVueModelBuilder::DComplexCircularBufferBus> A_Input;

	double NumOfXAntElement;
	double NumOfYAntElement;

	double Dx;
	double Dy;

	int    NumOfSamples;

	double Theta;
	double Phi;

	double SampleRate;


//private:
	int getNumElements() const;

	bool hasElPort();
	bool hasAzPort();

	int  getUsableChannelCount(int expectedM);
	bool setupOutputBusRate(int nCh);

	bool runSingleChannelBlackBoxBranch(int expectedM);

	void buildSteeringVector(int nx,
		int ny,
		double dx,
		double dy,
		double thetaDeg,
		double phiDeg,
		std::vector< std::complex<double> >& a) const;

	bool solveLinearSystem(
		std::vector< std::vector< std::complex<double> > > A,
		const std::vector< std::complex<double> >& b,
		std::vector< std::complex<double> >& x) const;

	void fallbackConventionalWeight(
		const std::vector< std::complex<double> >& a,
		std::vector< std::complex<double> >& w) const;

	static double deg2rad(double x);
};
