#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Compress : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedCompressionType { MULaw, ALaw };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Compress );

	// Constructor to initialize parameters
	Compress();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	SelectedCompressionType CompressionType;
	double CompressionK;
	double Max;
};
