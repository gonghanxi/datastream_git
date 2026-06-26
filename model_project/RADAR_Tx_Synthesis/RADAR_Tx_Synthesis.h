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
 * RADAR_Tx_Synthesis
 *
 * 功能：
 *   RF transmitter antenna array synthesis。
 *   根据帮助文档，本模型用于将平面阵列各输入阵元信号合成为远场观察点处的电场。
 *
 * 帮助文档关键点：
 *   - Domain: Timed
 *   - input    : multiple envelope，bus 宽度为 NumOfAntx * NumOfAnty
 *   - InTheta  : optional real，单位 radians
 *   - InPhi    : optional real，单位 radians
 *   - output   : envelope，每次 firing 产生 1 个 token
 *   - AntPhase : real，每次 firing 产生 NumOfAntx * NumOfAnty 个 token，单位 radians
 *   - 相位计算方式与 RADAR_PhaseShift 相同
 *
 * 与 RADAR_PhaseShift 的关系：
 *   RADAR_PhaseShift: 单路 envelope -> 多路 multiple envelope
 *   RADAR_Tx_Synthesis: 多路 multiple envelope -> 单路 envelope
 */
class RADAR_Tx_Synthesis : public SystemVueModelBuilder::TimedDFModel
{
public:
	typedef std::complex<double> Cx;
	typedef SystemVueModelBuilder::EnvelopeSignal EnvSig;

	typedef SystemVueModelBuilder::EnvelopeCircularBuffer EnvBuf;
	typedef SystemVueModelBuilder::EnvelopeCircularBufferBus EnvBus;
	typedef SystemVueModelBuilder::DoubleCircularBuffer RealBuf;

	DECLARE_MODEL_INTERFACE(RADAR_Tx_Synthesis);

	RADAR_Tx_Synthesis();

	bool Setup() override;
	bool Run() override;

	// 用于尽量继承输入 multiple envelope 第 0 路的时间轴。
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
	// Port 1：input，multiple envelope 输入，bus 宽度 N=NumOfAntx*NumOfAnty
	// Port 2：InTheta，可选 real 输入，单位 radians
	// Port 3：InPhi，可选 real 输入，单位 radians
	// Port 4：output，单路 envelope 输出
	// Port 5：AntPhase，real 输出，单位 radians，rate=N
	// ============================================================
	EnvBus input;
	RealBuf InTheta;
	RealBuf InPhi;
	EnvBuf output;
	RealBuf AntPhase;

	// ============================================================
	// RADAR_Tx_Synthesis 帮助文档参数
	// 与 RADAR_PhaseShift 参数保持一致
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
	int inputBusSize_;

	double inputStartTime_;
	double inputTimeStep_;
	double inputFc_;

	std::vector<double> phaseCacheRad_;

private:
	bool validateAndPrepare_();
	void buildPhaseTable_();

	double getThetaRad_();
	double getPhiRad_();

	double computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const;
	double getDesiredPhaseRad_(int index) const;

	Cx phaseRotator_(double phaseRad) const;

	void applyInputRates_();
	void applyOutputTiming_(double startTime);
	void applyOutputFc_();

	static double deg2rad(double x);
	static double clampFinite(double x, double fallback);
};
