#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

#include <deque>
#include <complex>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API IntegratorCx : public SystemVueModelBuilder::TimedDFModel
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

	DECLARE_MODEL_INTERFACE(IntegratorCx);
	IntegratorCx();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<int>                  reset;
	SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>> data;
	SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>> output;

	IntegrationMethodEnum IntegrationMethod;   
	LimitOutputEnum       LimitOutput;         

	double Top;                
	double Bottom;             
	std::complex<double> InitialState; 

	WindowEnum UseIntegrationWindow; 
	double     FeedbackGain;         
	double     IntegrationTime;      
	int        IntegrationSamples;   

//private:
	using Complex = std::complex<double>;

	double Ts_;           
	double lastTime_;     
	bool   haveLastTime_; 

	Complex state_;       
	Complex prevInput_;   

	bool   resetConnected_; 

	std::deque<Complex> areaWindow_;   
	std::deque<double>  timeWindow_;   

	Complex computeArea(const Complex& xPrev, const Complex& xCurr, double Ts);
	void    applyLimits();
};
