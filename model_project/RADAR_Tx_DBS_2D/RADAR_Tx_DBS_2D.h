#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "CircularBuffer.h"

#include <complex>
#include <vector>
#include <cmath>
#include <algorithm>

class SYSTEMVUEMODELBUILDER_API RADAR_Tx_DBS_2D : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum Window_TypeEnum
	{
		Rectangle = 0,
		Bartlett = 1,
		Hanning = 2,
		Hamming = 3,
		Blackman = 4,
		SteepBlackman = 5,
		Kaiser = 6
	};

	DECLARE_MODEL_INTERFACE(RADAR_Tx_DBS_2D);
	RADAR_Tx_DBS_2D();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>> input; 
	SystemVueModelBuilder::TimedCircularBuffer<double> InTheta;             
	SystemVueModelBuilder::TimedCircularBuffer<double> InPhi;               

	using Cx = std::complex<double>;
	using CxStream = SystemVueModelBuilder::TimedCircularBuffer<Cx>;
	using CxBus = SystemVueModelBuilder::CircularBufferBusT<CxStream>;
	CxBus output; 

	int    NumOfAntx;          
	int    NumOfAnty;          
	double Dx;                 
	double Dy;                 
	double Theta;              
	double Phi;                
	Window_TypeEnum Window_Type;
	double WindowParameters;   

private:
	static constexpr double kPi = 3.1415926535897932384626433832795;
	static constexpr double kTwoPi = 6.283185307179586476925286766559;

	int nx_ = 1;
	int ny_ = 1;
	int nChExpected_ = 1;   

	bool thetaConnected_ = false;
	bool phiConnected_ = false;

	std::vector<double> xPos_;     // x_m = m*Dx（corner-origin）
	std::vector<double> yPos_;     // y_n = n*Dy
	std::vector<double> wx_;       // X窗
	std::vector<double> wy_;       // Y窗
	std::vector<double> taper2d_;  // 展开的二维taper（row-major：x快变）

	static double deg2rad(double deg) { return deg * kPi / 180.0; }

	static double i0_bessel(double x);
	static void make_window(Window_TypeEnum type, int L, double beta, std::vector<double>& w);

	void rebuild_cache_();
};
