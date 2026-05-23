#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include <cfloat>   
#include <cmath>

class SYSTEMVUEMODELBUILDER_API DB : public SystemVueModelBuilder::DFModel
{
public:
	enum DbTypeEnum
	{
		POWER = 0,     
		AMPLITUDE = 1  
	};

	DECLARE_MODEL_INTERFACE(DB);

	DB();

	SystemVueModelBuilder::CircularBuffer<double> input;
	SystemVueModelBuilder::CircularBuffer<double> output;

	double     Min;    
	DbTypeEnum DbType; 

	bool Initialize();
	bool Run();
	bool Finalize();
	bool UpdateDynamicParameters();
};
