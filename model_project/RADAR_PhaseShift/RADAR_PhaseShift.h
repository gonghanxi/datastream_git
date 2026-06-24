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

class RADAR_PhaseShift : public SystemVueModelBuilder::TimedDFModel
{
public:
	typedef std::complex<double> Cx;
	typedef SystemVueModelBuilder::EnvelopeSignal EnvSig;

	typedef SystemVueModelBuilder::EnvelopeCircularBuffer EnvBuf;
	typedef SystemVueModelBuilder::EnvelopeCircularBufferBus EnvBus;
	typedef SystemVueModelBuilder::DoubleCircularBuffer RealBuf;

	DECLARE_MODEL_INTERFACE(RADAR_PhaseShift);

	RADAR_PhaseShift();

	bool Setup() override;
	bool Run() override;

	// 用于尽量继承输入 envelope 的时间轴。
	ERESULT CalculateLatency() override;

	// 用于传播 envelope 的载频 Fc。
	ERESULT PropagateCharacterizationFrequency() override;

	// ============================================================
	// 参数 Type 枚举
	// 0：根据 theta / phi 计算阵列相位
	// 1：直接使用 DesiredPhaseShift 参数数组
	// ============================================================
	enum PhaseShiftTypeEnum
	{
		Calculate_by_theta_and_phi = 0,
		DesiredPhaseShiftType = 1
	};

	// ============================================================
	// 端口定义
	// Port 1：Input，单路 envelope 输入
	// Port 2：InTheta，可选 real 输入，单位 radians
	// Port 3：InPhi，可选 real 输入，单位 radians
	// Port 4：output，multiple envelope 输出
	// Port 5：AntPhase，real 输出，单位 radians
	// ============================================================
	EnvBuf  Input;
	RealBuf InTheta;
	RealBuf InPhi;
	EnvBus  output;
	RealBuf AntPhase;

	// ============================================================
	// RADAR_PhaseShift 帮助文档参数
	// ============================================================
	int NumOfAntx;
	int NumOfAnty;
	PhaseShiftTypeEnum Type;

	double Dx;
	double Dy;
	double Theta;
	double Phi;

	// SystemVue 数组参数：需要配套 Size 成员。
	// 注意：该 Size 使用 int，可避免 AddParamArray 重载不匹配。
	double* DesiredPhaseShift;
	int     DesiredPhaseShift_Size;

private:
	int nAnt_;
	int outBusSize_;

	double inputStartTime_;
	double inputTimeStep_;
	double inputFc_;

	std::vector<double> phaseCacheRad_;

private:
	bool validateAndPrepare_();
	void buildPhaseTable_();

	// 这两个函数不能声明为 const：
	// SystemVue 2020 的 CircularBufferBase::IsConnected() 不是 const 成员函数。
	double getThetaRad_();
	double getPhiRad_();

	double computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const;
	double getDesiredPhaseRad_(int index) const;

	Cx phaseRotator_(double phaseRad) const;

	void applyOutputTiming_(double startTime);
	void applyOutputFc_();

	static double deg2rad(double x);
	static double clampFinite(double x, double fallback);
};
