#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

#include <deque>
#include <limits>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API Integrator : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum IntegrationMethodEnum
	{
		RECTANGLE = 0,
		TRAPEZOIDAL = 1
	};

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

	DECLARE_MODEL_INTERFACE(Integrator);
	Integrator();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<int>    reset;
	SystemVueModelBuilder::TimedCircularBuffer<double> data;
	SystemVueModelBuilder::TimedCircularBuffer<double> output;

	IntegrationMethodEnum IntegrationMethod;   
	LimitOutputEnum       LimitOutput;         
	double                Top;                
	double                Bottom;             
	double                InitialState;       
	WindowEnum            UseIntegrationWindow; 
	double                FeedbackGain;       
	double                IntegrationTime;    
	int                   IntegrationSamples; 

//private:
	double Ts_;           
	double lastTime_;     
	bool   haveLastTime_; 

	double state_;        
	double prevInput_;    

	bool   resetConnected_; 

	std::deque<double> areaWindow_;   
	std::deque<double> timeWindow_;   

	double computeArea(double xPrev, double xCurr, double Ts);
	void   applyLimits();
};
