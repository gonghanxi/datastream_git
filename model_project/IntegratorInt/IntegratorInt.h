#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

#include <deque>
#include <limits>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API IntegratorInt : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum LimitOutputEnum
	{
		LIMIT_NO = 0,
		LIMIT_SATURATE = 1,
		LIMIT_WRAP = 2
	};

	enum WindowEnum
	{
		WIN_NO = 0,
		WIN_DEFINED_IN_TIME = 1,
		WIN_DEFINED_IN_SAMPLES = 2
	};

	DECLARE_MODEL_INTERFACE(IntegratorInt);
	IntegratorInt();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<int> reset;
	SystemVueModelBuilder::TimedCircularBuffer<int> data;
	SystemVueModelBuilder::TimedCircularBuffer<int> output;

	LimitOutputEnum LimitOutput;          
	int             Top;                  
	int             Bottom;               
	int             InitialState;         
	WindowEnum      UseIntegrationWindow; 
	double          IntegrationTime;      
	int             IntegrationSamples;   

//private:
	long long state_;        
	bool      haveState_;    
	bool      resetConnected_;

	std::deque<int>    valueWindow_;
	std::deque<double> timeWindow_;

	void applyLimits();
};
