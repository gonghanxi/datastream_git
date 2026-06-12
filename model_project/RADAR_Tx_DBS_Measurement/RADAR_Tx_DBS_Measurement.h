#pragma once

#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "CircularBuffer.h"

#include <complex>
#include <vector>
#include <algorithm>

/*
 * RADAR_Tx_DBS_Measurement
 *
 * 功能：
 *   RF transmitter antenna pattern measurement。
 *   根据帮助文档，本模型用于计算发射矩形阵列天线的方向图。
 *   在一个 firing 中读取一个 PRI 长度内的多路发射阵元 envelope 信号，
 *   对 Theta 或 Phi 做线性扫描，并输出对应角度下的电压平方方向图点。
 *
 * 帮助文档关键点：
 *   - Domain: Timed
 *   - input      : multiple envelope，bus 宽度理论上为 NumOfAntx * NumOfAnty
 *   - AntPattern : complex，输出方向图复数点
 *   - 每个 input bus lane 每次 firing 消耗 PRI * SamplingRate 个 token，默认 1000
 *   - AntPattern 每次 firing 输出 Nsamples 个 token，默认 360
 *   - ParamToSweep / TypeOfSweep 两个枚举不导致任何参数显隐变化
 *
 * 公式按帮助文档实现为：
 *   Output(k) = abs( (1/(PRI*SamplingRate)) * sum_i sum_j input[j][i] * Rotation[j][k] )^2
 *               * exp(j * (SweepStart + k * SweepStep))
 *
 *   Rotation[kx + NumOfAntx * ky][k] = exp(j * phase(kx,ky,theta,phi))
 *
 *   phase(kx,ky) = 2*pi * [kx*Dx*sin(theta)*cos(phi)
 *                         + ky*Dy*sin(theta)*sin(phi)]
 *
 * 说明：
 *   1. Dx / Dy 单位为 wavelength，因此相位计算中直接乘 2*pi。
 *   2. 角度参数设置 Units::ANGLE 后，C++ 成员值按 SystemVue 习惯作为 radians 使用，
 *      代码中不再对 Theta_Phi / SweepStart / SweepStop / SweepStepSize 做二次 deg2rad。
 *   3. 输出为 complex，是为了将方向图幅值平方映射到扫描角对应的复平面点。
 */
class SYSTEMVUEMODELBUILDER_API RADAR_Tx_DBS_Measurement : public SystemVueModelBuilder::TimedDFModel
{
public:
	typedef std::complex<double> Cx;
	typedef SystemVueModelBuilder::EnvelopeSignal EnvSig;
	typedef SystemVueModelBuilder::EnvelopeCircularBufferBus EnvBus;
	typedef SystemVueModelBuilder::DComplexCircularBuffer CxBuf;

	DECLARE_MODEL_INTERFACE(RADAR_Tx_DBS_Measurement);

	RADAR_Tx_DBS_Measurement();

	bool Setup() override;
	bool Run() override;

	// Output is complex (not envelope), no Fc propagation needed;
	// Keep CalculateLatency to drive the timeline per TimedDFModel convention.
	ERESULT CalculateLatency() override;

	// ============================================================
	// ParamToSweep enum
	// 0: Sweep Phi   Theta_Phi is used as fixed theta
	// 1: Sweep Theta Theta_Phi is used as fixed phi
	// ============================================================
	enum ParamToSweepEnum
	{
		Sweep_Phi = 0,
		Sweep_Theta = 1
	};

	// ============================================================
	// TypeOfSweep enum
	// 0: Linear:Number of Points
	// 1: Linear:Step Size
	// ============================================================
	enum TypeOfSweepEnum
	{
		Linear_Number_of_Points = 0,
		Linear_Step_Size = 1
	};

	// ============================================================
	// Port definitions
	// Port 1: input,   multiple envelope input
	// Port 2: AntPattern, complex output
	// ============================================================
	EnvBus input;
	CxBuf  AntPattern;

	// ============================================================
	// RADAR_Tx_DBS_Measurement help document parameters
	// ============================================================
	double PRI;
	double SamplingRate;

	int NumOfAntx;
	int NumOfAnty;

	double Dx;
	double Dy;

	ParamToSweepEnum ParamToSweep;
	double Theta_Phi;

	TypeOfSweepEnum TypeOfSweep;
	double SweepStart;
	double SweepStop;
	int    SweepNumOfPoints;
	double SweepStepSize;


	int nAnt_;
	int inputRate_;
	int sweepSamples_;

	double sweepStepRad_;
	double inputStartTime_;
	double inputTimeStep_;


	bool validateAndPrepare_();
	void applyInputRates_();

	int computeInputRate_() const;
	int computeSweepSamples_() const;
	double computeSweepStepRad_(int nSamples) const;

	void getThetaPhiForSweep_(int k, double& thetaRad, double& phiRad, double& sweepAngleRad) const;
	double computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const;
	Cx phaseRotator_(double phaseRad) const;

	Cx getInputCx_(int antIndex, int sampleIndex);

	static double deg2rad(double x);
	static double clampFinite(double x, double fallback);
	static int    clampInt(int x, int lo, int hi);
};
