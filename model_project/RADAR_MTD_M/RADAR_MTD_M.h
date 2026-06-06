#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"

#include <vector>
#include <complex>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API RADAR_MTD_M : public SystemVueModelBuilder::DFModel
{
public:
	// 窗函数类型枚举：顺序与帮助文档一致
	enum SelectedWindowType {
		Rectangle = 0,      // 矩形窗
		Bartlett = 1,       // 三角窗
		Hanning = 2,        // 汉宁窗
		Hamming = 3,        // 汉明窗
		Blackman = 4,       // 布莱克曼窗
		SteepBlackman = 5,  // 陡峭布莱克曼窗
		Kaiser = 6          // 凯泽窗
	};

public:
	// SystemVue 模型必需宏
	DECLARE_MODEL_INTERFACE(RADAR_MTD_M);

	RADAR_MTD_M();
	virtual ~RADAR_MTD_M();

	virtual bool Setup();
	virtual bool Run();

	// ===== 端口 =====
	// input  : complex matrix
	// output : complex matrix
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< std::complex<double> > > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< std::complex<double> > > output;

	// ===== 参数：参考 RADAR_MTD_M 帮助文档 =====
	int NumOfPulse;                         // Number of pulses
	double* Freq_Weight;                    // The weights in frequency domain
	int Freq_Weight_Size;                   // Freq_Weight 数组长度
	SelectedWindowType WindowType;          // The type of window function
	double* WindowParameters;               // The array of values for the window
	int WindowParameters_Size;              // WindowParameters 数组长度

private:
	static constexpr double kPI = 3.14159265358979323846;

	// 窗函数
	std::vector<double> window;

private:
	// ===== MTD 核心处理 =====
	void processOneSlowTimeVector(std::vector<std::complex<double>>& x);

	// ===== FFT / DFT：纯 C++ 实现，不依赖 FFTW =====
	bool isPowerOfTwo(int n) const;
	void internalFFT(std::vector<std::complex<double>>& x);
	void backupFFT(std::vector<std::complex<double>>& x);
	void directDFT(std::vector<std::complex<double>>& x);

	// ===== 窗函数生成 =====
	void generateWindow(int size);
	std::vector<double> generateBartlettWindow(int size);
	std::vector<double> generateHanningWindow(int size);
	std::vector<double> generateHammingWindow(int size);
	std::vector<double> generateBlackmanWindow(int size);
	std::vector<double> generateSteepBlackmanWindow(int size);
	std::vector<double> generateKaiserWindow(int size, double beta);
	double modifiedBesselI0(double x);
};
