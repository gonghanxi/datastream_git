#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <complex>

class RADAR_PulseCompression_M : public SystemVueModelBuilder::DFModel
{
public:
	// 窗函数枚举顺序与普通版 RADAR_PulseCompression 以及帮助文档保持一致。
	enum SelectedWindowType
	{
		Rectangle = 0,
		Bartlett = 1,
		Hanning = 2,
		Hamming = 3,
		Blackman = 4,
		SteepBlackman = 5,
		Kaiser = 6
	};

public:
	typedef std::complex<double> Cx;
	typedef SystemVueModelBuilder::Matrix<Cx> CxMatrix;

	DECLARE_MODEL_INTERFACE(RADAR_PulseCompression_M);

	RADAR_PulseCompression_M();

	virtual bool Setup();
	virtual bool Run();

	// 递归 FFT，尽量保持与普通版 RADAR_PulseCompression 相同的实现方式。
	// invert = 1  ：执行普通版代码中的“FFT”方向。
	// invert = -1 ：执行普通版代码中的“IFFT”方向，并在递归中完成 1/N 缩放。
	void fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert);

	// Kaiser 窗近似计算辅助函数，保持普通版逻辑不变。
	int factorial(int n);
	double I0(int n, double x);

	// ============================================================
	// 端口定义
	// Port 1：reference，complex matrix，匹配滤波器频谱 H[k]
	// Port 2：signal，complex matrix，待脉冲压缩的输入信号矩阵
	// Port 3：output，complex matrix，脉冲压缩结果矩阵
	// ============================================================
	SystemVueModelBuilder::CircularBuffer<CxMatrix> reference;
	SystemVueModelBuilder::CircularBuffer<CxMatrix> signal;
	SystemVueModelBuilder::CircularBuffer<CxMatrix> output;

	// ============================================================
	// RADAR_PulseCompression_M 帮助文档参数
	// ============================================================
	SelectedWindowType WindowType;
	double WindowParameter;

private:
	// 根据 reference 矩阵尺寸推断 FFTSize。
	// 常规 Pack_M / DynamicPack_M 用法下，reference 为 1×FFTSize 或 N×FFTSize。
	// 如果 reference 被构造成列向量，则兼容为 FFTSize×1。
	int getReferenceFFTSize_(const CxMatrix& ref) const;

	// 读取第 row 行、第 k 个频点的 reference。
	// 如果 reference 只有 1 行，则所有 signal 行共用第 0 行 reference。
	Cx getReferenceValue_(const CxMatrix& ref, int row, int k) const;

	// 生成频域加窗序列。矩阵版没有 Bandwidth/SampleRate 参数，
	// 因此窗口长度按 FFTSize 处理，其余公式沿用普通版。
	void buildWindowSequence_(SystemVueModelBuilder::Matrix<std::complex<double>>& windowSeq,
		int fftSize);
};
