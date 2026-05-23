#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API RADAR_CFAR : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedCFARType{ CA, SOCA, GOCA, OS, ClutterMap };
    enum SelectedDetectorType{ Envelope, Square, LogSquare, Log };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_CFAR );

	// Constructor to initialize parameters
	RADAR_CFAR();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	double SOFactor(double alpha, int N);
	double GOFactor(double alpha, int N);
	//double OSFactor(double alpha, int N, double k);
	double ClutterMapPointFactor(double alpha, int m, double r);
	double ClutterMapPlaneFactor(double alpha, int m, double r, double M);
	double SolutionBinaray(double Pf, int N, double a, double b, double precision, SelectedCFARType CFARType);

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output, threshold;
	
	// Parameter
	SelectedCFARType CFARType;
	int CellSize;
	int ReferenceCell;
	int GuardCell;
	int kOrder;
	double ThresholdScaleFactor;
	SelectedDetectorType DetectorType;
	double Pf;
	double Alpha;
	double Beta;

	double ThresholdFactor;
};
