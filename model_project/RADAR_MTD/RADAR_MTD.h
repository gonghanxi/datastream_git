#pragma once
#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"
#include "EnvelopeSignal.h"
#include <vector>
#include <complex>

// 前向声明FFTW类型以避免在头文件中包含FFTW头文件
typedef struct fftw_plan_s* fftw_plan;
typedef double fftw_complex[2];

class SYSTEMVUEMODELBUILDER_API RADAR_MTD : public SystemVueModelBuilder::DFModel
{
public:
	// 窗函数类型枚举
	enum SelectedWindowType {
		Rectangle,      // 矩形窗
		Bartlett,       // 三角窗
		Hanning,        // 汉宁窗
		Hamming,        // 汉明窗
		Blackman,       // 布莱克曼窗
		SteepBlackman,  // 陡峭布莱克曼窗
		Kaiser          // 凯泽窗
	};

public:
	// SystemVue模型必需的宏
	DECLARE_MODEL_INTERFACE(RADAR_MTD);

	// 构造函数和析构函数
	RADAR_MTD();
	virtual ~RADAR_MTD();

	// 系统函数重写
	virtual bool Run();
	virtual bool Setup();

	// 输入输出端口
	SystemVueModelBuilder::DComplexCircularBuffer input, output;

	// 模型参数
	double PRI;                    // 脉冲重复间隔
	double SampleRate;             // 采样率
	int NumOfPulse;                // 脉冲数量
	SelectedWindowType WindowType;       // 窗函数类型
	double* Freq_Weight;           // 频率权重数组
	int Freq_Weight_Size;          // 频率权重数组大小
	double* WindowParameters;      // 窗函数参数数组
	int WindowParameters_Size;     // 窗函数参数数组大小

private:
	// 内部变量
	int samplesPerPulse;           // 每个脉冲的采样点数
	int totalSamples;              // 总采样点数

	// 数据缓冲区
	std::vector<std::complex<double>> inputBuffer;     // 输入数据缓冲区
	std::vector<std::complex<double>> outputBuffer;    // 输出数据缓冲区
	std::vector<std::complex<double>> pulseMatrix;     // 脉冲矩阵
	std::vector<std::complex<double>> cancelledData;   // 对消后数据
	std::vector<double> window;                        // 窗函数系数

	// FFTW相关变量
	fftw_plan fft_plan;            // FFT计划
	fftw_plan ifft_plan;           // IFFT计划
	fftw_complex* fftw_input;      // FFT输入数组
	fftw_complex* fftw_output;     // FFT输出数组
	bool fftw_initialized;         // FFTW初始化标志

	// FFT相关函数
	bool InitializeFFTW();                                                            // 初始化FFTW
	bool ExternalFFT(std::vector<std::complex<double>>& x);                          // 外部FFT变换
	bool ExternalIFFT(std::vector<std::complex<double>>& x);                         // 外部IFFT变换
	void BackupFFT(std::vector<std::complex<double>>& x);                            // 备用FFT实现
	void BackupIFFT(std::vector<std::complex<double>>& x);                           // 备用IFFT实现

	// 窗函数生成函数
	void generateWindow();                                                            // 生成窗函数
	std::vector<double> generateBartlettWindow(int size);                            // 生成三角窗
	std::vector<double> generateHanningWindow(int size);                             // 生成汉宁窗
	std::vector<double> generateHammingWindow(int size);                             // 生成汉明窗
	std::vector<double> generateBlackmanWindow(int size);                            // 生成布莱克曼窗
	std::vector<double> generateSteepBlackmanWindow(int size);                       // 生成陡峭布莱克曼窗
	std::vector<double> generateKaiserWindow(int size, double beta);                 // 生成凯泽窗
	double modifiedBesselI0(double x);                                               // 计算修正的零阶贝塞尔函数
};
