#pragma once
#include "ModelBuilder.h"

class RADAR_SAR_Echo : public SystemVueModelBuilder::DFModel
{

public:
    enum SelectedSAR_Mode{ Stripmap };
    enum SelectedEchoGenerate_Mode{ Point_Target };
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_SAR_Echo );

	// Constructor to initialize parameters
	RADAR_SAR_Echo();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > output;

    std::vector<std::complex<double>> outputData;
	
	// Parameter
	SelectedSAR_Mode SAR_Mode;
	double Fc;
	double Xmin;
	double Xmax;
	double Yc;
	double Y0;
	double H;
	double Vr;
	double D;
	double Tr;
	double Br;
	double SampleRate;
	SelectedEchoGenerate_Mode EchoGenerate_Mode;
	SystemVueModelBuilder::Matrix<double> TargetInfo;

    double m_Nslow;
    double m_Nfast;

private:
	double lambda;
	double R0;
	double Lsar;
	double Tsar;
	double Ka;
	double Ba;
	double PRF;
	double PRI;
	double ds;
	int Nslow;
	double Kr;
	double Fsr;
	double dt;
	double Rmin;
	double Rmax;
	int Nfast;
};
