#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "CircularBuffer.h"

#include <complex>
#include <vector>
#include <cmath>
#include <algorithm>

class SYSTEMVUEMODELBUILDER_API RADAR_Rx_DBS_2D : public SystemVueModelBuilder::TimedDFModel
{
public:
	// 下拉枚举名必须与显示一致
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

	DECLARE_MODEL_INTERFACE(RADAR_Rx_DBS_2D);
	RADAR_Rx_DBS_2D();

	bool Setup() override;
	bool Run()   override;

	// ===== 端口 =====
	using Cx = std::complex<double>;
	using CxStream = SystemVueModelBuilder::TimedCircularBuffer<Cx>;
	using CxBus = SystemVueModelBuilder::CircularBufferBusT<CxStream>;

	CxBus input; // 绿色总线复数输入（multiple complex）
	SystemVueModelBuilder::TimedCircularBuffer<double> InTheta; // 蓝色可选输入（rad）
	SystemVueModelBuilder::TimedCircularBuffer<double> InPhi;   // 蓝色可选输入（rad）

	SystemVueModelBuilder::TimedCircularBuffer<Cx> output;      // 绿色复数输出（single complex）

	// ===== 参数 =====
	int    NumOfAntx;          // Number of Antenna in X axis
	int    NumOfAnty;          // Number of Antenna in Y axis
	double Dx;                 // Antenna Spacing in wavelengths of X axis
	double Dy;                 // Antenna Spacing in wavelengths of Y axis
	double Theta;              // deg（界面对齐）
	double Phi;                // deg（界面对齐）
	Window_TypeEnum Window_Type;
	double WindowParameters;   // Kaiser Beta

private:
	static constexpr double kPi = 3.1415926535897932384626433832795;
	static constexpr double kTwoPi = 6.283185307179586476925286766559;

	int nx_ = 1;
	int ny_ = 1;
	int nChExpected_ = 1;          // 期望输入通道数 Nx*Ny

	bool thetaConnected_ = false;
	bool phiConnected_ = false;

	std::vector<double> xPos_;     // x_m = m*Dx（corner-origin）
	std::vector<double> yPos_;     // y_n = n*Dy
	std::vector<double> wx_;       // X窗
	std::vector<double> wy_;       // Y窗
	std::vector<double> taper2d_;  // 展开的二维taper（row-major：x快变）

	static double deg2rad(double deg) { return deg * kPi / 180.0; }

	// Kaiser I0：VS2017 兼容实现（与 Tx 保持一致）
	static double i0_bessel(double x);

	static void make_window(Window_TypeEnum type, int L, double beta, std::vector<double>& w);

	void rebuild_cache_();
};
