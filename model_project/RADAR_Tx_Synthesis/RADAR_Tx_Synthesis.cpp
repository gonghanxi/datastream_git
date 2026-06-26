#include "RADAR_Tx_Synthesis.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Tx_Synthesis)
{
	SET_MODEL_DESCRIPTION("RF transmitter antenna array synthesis");
	SET_MODEL_CATEGORY("Environments");

	// ============================================================
	// 端口
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("The input signal");
	}

	{
		auto p = ADD_MODEL_INPUT(InTheta);
		p.SetName("InTheta");
		p.SetDescription("The array direction angle theta in radians");
		p.SetOptional(true);
	}

	{
		auto p = ADD_MODEL_INPUT(InPhi);
		p.SetName("InPhi");
		p.SetDescription("The array direction angle phi in radians");
		p.SetOptional(true);
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output signals");
	}

	{
		auto p = ADD_MODEL_OUTPUT(AntPhase);
		p.SetName("AntPhase");
		p.SetDescription("Test for phase output in radians");
	}

	// ============================================================
	// 参数
	// ============================================================
	{
		auto p = ADD_MODEL_PARAM(NumOfAntx);
		p.SetName("NumOfAntx");
		p.SetDefaultValue("4");
		p.SetDescription("Number of Antenna in X axis");
	}

	{
		auto p = ADD_MODEL_PARAM(NumOfAnty);
		p.SetName("NumOfAnty");
		p.SetDefaultValue("4");
		p.SetDescription("Number of Antenna in Y axis");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(Type, RADAR_Tx_Synthesis::PhaseShiftTypeEnum);
		p.SetName("Type");
		p.AddEnumeration("Calculate by theta and phi", RADAR_Tx_Synthesis::Calculate_by_theta_and_phi);
		p.AddEnumeration("DesiredPhaseShift", RADAR_Tx_Synthesis::DesiredPhaseShiftType);
		p.SetDefaultValue("Calculate by theta and phi");
		p.SetDescription("The phase shift value calculation method: Calculate by theta and phi, DesiredPhaseShift");
	}

	{
		auto p = ADD_MODEL_PARAM(Dx);
		p.SetName("Dx");
		p.SetDefaultValue("0.5");
		p.SetDescription("Antenna Spacing in wavelengths of X axis");
	}

	{
		auto p = ADD_MODEL_PARAM(Dy);
		p.SetName("Dy");
		p.SetDefaultValue("0.5");
		p.SetDescription("Antenna Spacing in wavelengths of Y axis");
	}

	{
		auto p = ADD_MODEL_PARAM(Theta);
		p.SetName("Theta");
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The target direction angle which is the angle of scan measured from broadside of antenna");
	}

	{
		auto p = ADD_MODEL_PARAM(Phi);
		p.SetName("Phi");
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The target direction angle which is the angle of scan measured from the x-axis of antenna");
	}

	{
		// SystemVue 2020 数组参数写法：第二个参数必须是 Size 成员。
		// DesiredPhaseShift_Size 使用 int 可通过重载匹配。
		auto p = ADD_MODEL_ARRAY_PARAM(DesiredPhaseShift, DesiredPhaseShift_Size);
		p.SetName("DesiredPhaseShift");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]");
		p.SetDescription("Desired phase shift angles");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_Tx_Synthesis::RADAR_Tx_Synthesis()
	: input()
	, InTheta()
	, InPhi()
	, output()
	, AntPhase()
	, NumOfAntx(4)
	, NumOfAnty(4)
	, Type(Calculate_by_theta_and_phi)
	, Dx(0.5)
	, Dy(0.5)
	, Theta(0.0)
	, Phi(0.0)
	, DesiredPhaseShift(nullptr)
	, DesiredPhaseShift_Size(0)
	, nAnt_(16)
	, inputBusSize_(0)
	, inputStartTime_(0.0)
	, inputTimeStep_(0.0)
	, inputFc_(0.0)
{
}

bool RADAR_Tx_Synthesis::validateAndPrepare_()
{
	if (NumOfAntx < 1) NumOfAntx = 1;
	if (NumOfAnty < 1) NumOfAnty = 1;

	nAnt_ = NumOfAntx * NumOfAnty;
	if (nAnt_ < 1) nAnt_ = 1;

	phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);

	inputBusSize_ = static_cast<int>(input.GetSize());

	if (inputBusSize_ > 0)
	{
		inputStartTime_ = input[0].GetStartTime();
		inputTimeStep_ = input[0].GetTimeStep();
		inputFc_ = input[0].GetCharacterizationFrequency();
	}
	else
	{
		inputStartTime_ = 0.0;
		inputTimeStep_ = 0.0;
		inputFc_ = 0.0;
	}

	return true;
}

void RADAR_Tx_Synthesis::applyInputRates_()
{
	const size_t busSize = input.GetSize();
	for (size_t k = 0; k < busSize; ++k)
	{
		input[k].SetRate(1u);
	}
}

void RADAR_Tx_Synthesis::applyOutputTiming_(double startTime)
{
	output.SetStartTime(startTime);
	if (inputTimeStep_ > 0.0)
	{
		output.SetTimeStep(inputTimeStep_);
	}
}

void RADAR_Tx_Synthesis::applyOutputFc_()
{
	output.SetCharacterizationFrequency(inputFc_);
}

ERESULT RADAR_Tx_Synthesis::CalculateLatency()
{
	inputBusSize_ = static_cast<int>(input.GetSize());
	if (inputBusSize_ > 0)
	{
		inputStartTime_ = input[0].GetStartTime();
		inputTimeStep_ = input[0].GetTimeStep();
	}

	applyOutputTiming_(inputStartTime_);
	return (ERESULT)0;
}

ERESULT RADAR_Tx_Synthesis::PropagateCharacterizationFrequency()
{
	inputBusSize_ = static_cast<int>(input.GetSize());
	if (inputBusSize_ > 0)
	{
		inputFc_ = input[0].GetCharacterizationFrequency();
	}

	applyOutputFc_();
	return (ERESULT)0;
}

bool RADAR_Tx_Synthesis::Setup()
{
	validateAndPrepare_();

	// 帮助文档：input 是 bus，宽度为 NumOfAntx*NumOfAnty；
	// 每个 lane 每次 firing 消耗 1 个 token。
	applyInputRates_();

	if (InTheta.IsConnected())
	{
		InTheta.SetRate(1u);
	}
	if (InPhi.IsConnected())
	{
		InPhi.SetRate(1u);
	}

	// 帮助文档：output 每次 firing 产生 1 个 token。
	output.SetRate(1u);

	// 帮助文档：AntPhase 每次 firing 产生 NumOfAntx*NumOfAnty 个 token。
	AntPhase.SetRate(static_cast<unsigned>(nAnt_));

	buildPhaseTable_();

	applyOutputTiming_(inputStartTime_);
	applyOutputFc_();

	return true;
}

bool RADAR_Tx_Synthesis::Run()
{
	inputBusSize_ = static_cast<int>(input.GetSize());

	// 驱动 timed 模型时间轴，使用 input bus 第 0 路作为参考时间轴。
	if (inputBusSize_ > 0)
	{
		(void)input[0].GetTime(0, GetCount());

		inputStartTime_ = input[0].GetStartTime();
		inputTimeStep_ = input[0].GetTimeStep();
		inputFc_ = input[0].GetCharacterizationFrequency();
	}

	// 如果 InTheta / InPhi 是可选动态输入，每次 firing 都要重新计算相位。
	buildPhaseTable_();

	applyOutputTiming_(inputStartTime_);
	applyOutputFc_();

	// AntPhase 是 real 输出，帮助文档要求输出 NumOfAntx*NumOfAnty 个 token。
	for (int k = 0; k < nAnt_; ++k)
	{
		AntPhase[k] = phaseCacheRad_[static_cast<size_t>(k)];
	}

	// 帮助文档：本模型用于把平面阵列各输入信号合成为远场观察点电场。
	// 这里按最直接的阵列因子形式进行相干求和：
	//     y(t) = sum_i x_i(t) * exp(j * A_i)
	// 不做 1/N 或 1/sqrt(N) 归一化，因为帮助文档没有给出归一化参数。
	Cx acc(0.0, 0.0);

	const int nRead = std::min(inputBusSize_, nAnt_);
	for (int k = 0; k < nRead; ++k)
	{
		const EnvSig x = input[static_cast<size_t>(k)][0];
		const Cx xCx = x.complex();
		acc += xCx * phaseRotator_(phaseCacheRad_[static_cast<size_t>(k)]);
	}

	EnvSig y;
	y = acc;
	output[0] = y;

	return true;
}

void RADAR_Tx_Synthesis::buildPhaseTable_()
{
	if (nAnt_ <= 0)
	{
		nAnt_ = 1;
	}

	if (phaseCacheRad_.size() != static_cast<size_t>(nAnt_))
	{
		phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);
	}

	if (Type == DesiredPhaseShiftType)
	{
		for (int i = 0; i < nAnt_; ++i)
		{
			phaseCacheRad_[static_cast<size_t>(i)] = getDesiredPhaseRad_(i);
		}
		return;
	}

	const double thetaRad = getThetaRad_();
	const double phiRad = getPhiRad_();

	for (int ky = 0; ky < NumOfAnty; ++ky)
	{
		for (int kx = 0; kx < NumOfAntx; ++kx)
		{
			const int idx = ky * NumOfAntx + kx;
			if (idx >= 0 && idx < nAnt_)
			{
				phaseCacheRad_[static_cast<size_t>(idx)] = computePhaseRad_(kx, ky, thetaRad, phiRad);
			}
		}
	}
}

RADAR_Tx_Synthesis::Cx RADAR_Tx_Synthesis::phaseRotator_(double phaseRad) const
{
	return Cx(std::cos(phaseRad), std::sin(phaseRad));
}

double RADAR_Tx_Synthesis::getThetaRad_()
{
	// 帮助文档：可选输入端口 InTheta 的单位为 radians。
	if (InTheta.IsConnected())
	{
		return InTheta[0];
	}

	// 注意：Theta 在界面中显示为 degrees，但注册参数时设置了 Units::ANGLE。
	// SystemVue 会把角度参数转换成内部弧度值传给 C++ 成员变量。
	// 因此这里不能再 deg2rad(Theta)，否则会发生二次转换。
	return Theta;
}

double RADAR_Tx_Synthesis::getPhiRad_()
{
	// 帮助文档：可选输入端口 InPhi 的单位为 radians。
	if (InPhi.IsConnected())
	{
		return InPhi[0];
	}

	// Phi 同 Theta，C++ 成员变量中已经是 radians。
	return Phi;
}

double RADAR_Tx_Synthesis::computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const
{
	// 帮助文档公式与 RADAR_PhaseShift 一致：
	// theta(kx,ky) = (kx*dx/lambda)*sin(theta)*cos(phi)
	//              + (ky*dy/lambda)*sin(theta)*sin(phi)
	// 参数 Dx / Dy 的单位已经是 wavelengths，AntPhase 输出单位为 radians，
	// 因此这里乘 2*pi。
	const double sx = std::sin(thetaRad) * std::cos(phiRad);
	const double sy = std::sin(thetaRad) * std::sin(phiRad);

	const double cycles = static_cast<double>(kx) * Dx * sx
		+ static_cast<double>(ky) * Dy * sy;

	return 2.0 * M_PI * cycles;
}

double RADAR_Tx_Synthesis::getDesiredPhaseRad_(int index) const
{
	if (DesiredPhaseShift != nullptr &&
		DesiredPhaseShift_Size > 0 &&
		index >= 0 &&
		index < DesiredPhaseShift_Size)
	{
		// DesiredPhaseShift 在界面中显示为 degrees，
		// 但该数组参数也设置了 Units::ANGLE，因此 C++ 内部值已经是 radians。
		return DesiredPhaseShift[index];
	}

	return 0.0;
}

double RADAR_Tx_Synthesis::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_Tx_Synthesis::clampFinite(double x, double fallback)
{
	if (x != x)
	{
		return fallback;
	}

	if (!std::isfinite(x))
	{
		return fallback;
	}

	return x;
}
