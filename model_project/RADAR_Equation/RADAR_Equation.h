#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_Equation : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedEqType { Basic, CW, PD, Search, Track };
	enum SelectedOutputType { SNROut, RangeOut };
	enum SelectedAntennaType { Single, MonostaticSeparate };
	enum SelectedIntegrationType { Singlehit, Integration };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_Equation );

	// Constructor to initialize parameters
	RADAR_Equation();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	double	PowerRatioTodB(double PowerRatio);
	double	dBToPowerRatio(double dB);

	// Ports
	SystemVueModelBuilder::CircularBuffer< double >	output;
	
	// Parameter
	SelectedEqType EqType;
	SelectedOutputType OutputType;
	double Pt;
	double Pavg;
	double DwellTime;
	double PRF;
	SelectedAntennaType AntennaType;
	double Gain;
	double GainTx;
	double GainRx;
	double RCS;
	double NoiseFigure;
	double SystemNoiseTemperature;
	double Freq;
	double Pulsewidth;
	double Bandwidth;
	double SystemLoss;
	double PropagationLoss;
	double GroundPlaneLoss;
	double Range;
	double SNR;
	SelectedIntegrationType IntegrationType;
	double PulseNumber;
	double IntegrationLoss;
	double Theta3dB;
	double ScanRate;
	double ServoBandwidth;

};
