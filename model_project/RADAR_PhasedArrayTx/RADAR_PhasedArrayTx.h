#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_PhasedArrayTx : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedConfiguration { UniformLinearArray, UniformRectangularArray };
	enum SelectedAxisType { X, Y, Z };
	enum SelectedArray2DShapeType { Full, Customized };
	enum SelectedSpaceType { Uniform, NonUniform };
	enum SelectedGridType { Rectangular, Triangular };
	enum SelectedReliabilityType { NoFailures, RandomElement };
	enum SelectedWindowType { Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser, Taylor };
	enum SelectedYesorNo { Yes, No };
	enum SelectedPhaseShiftType { CalculateByThetaAndPhi, DesiredPhaseShift };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_PhasedArrayTx );

	// Constructor to initialize parameters
	RADAR_PhasedArrayTx();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBufferBus ArrayInput;
	SystemVueModelBuilder::DoubleCircularBuffer TargetThetaIn;
	SystemVueModelBuilder::DoubleCircularBuffer TargetPhiIn;
	SystemVueModelBuilder::DoubleCircularBuffer BeamThetaIn;
	SystemVueModelBuilder::DoubleCircularBuffer BeamPhiIn;
	SystemVueModelBuilder::EnvelopeCircularBufferBus ArrayOutput;

	// Parameter
	SelectedConfiguration Configuration;
	SelectedAxisType AxisType;
	SelectedArray2DShapeType Array2DShapeType;
	int NumOfAnt1D;
	int NumOfAnt2D_H;
	int NumOfAnt2D_V;
	double ElementFactor;
	SelectedSpaceType SpaceType;
	SelectedGridType GridType;
	double D;
	double D_H;
	double D_V;
	SystemVueModelBuilder::Matrix<double> D_array;
	SystemVueModelBuilder::Matrix<double> D_H_array;
	SystemVueModelBuilder::Matrix<double> D_V_array;
	SystemVueModelBuilder::Matrix<int> mask_array;
	SelectedReliabilityType ReliabilityType;
	double FailureProbability;
	double TargetTheta;
	double TargetPhi;
	SelectedWindowType WindowType;
	double KaiserWindowParameter;
	double Sidelobe_Levels;
	int nBar;
	SelectedYesorNo IsPhaseShift;
	double BeamTheta;
	double BeamPhi;
	SelectedYesorNo QuantizationType;
	int PhaseShifterBitwidth;
	SelectedPhaseShiftType PhaseShiftType;
	SystemVueModelBuilder::Matrix<double> DesiredPhaseShiftAngle;

private:
	double AntennaGain;
};
