#include "RADAR_PhaseShift.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_PhaseShift)
{
	SET_MODEL_DESCRIPTION("RF phase shifter continuously interpolated between time steps");
	SET_MODEL_CATEGORY("Environments");

	// ============================================================
	// 端口
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(Input);
		p.SetName("Input");
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
		p.SetDescription("The output signals after phase shift");
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
		auto p = ADD_MODEL_ENUM_PARAM(Type, RADAR_PhaseShift::PhaseShiftTypeEnum);
		p.SetName("Type");
		p.AddEnumeration("Calculate by theta and phi", RADAR_PhaseShift::Calculate_by_theta_and_phi);
		p.AddEnumeration("DesiredPhaseShift", RADAR_PhaseShift::DesiredPhaseShiftType);
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
		// 用户当前工程中 DesiredPhaseShift_Size 使用 int 可通过重载匹配。
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
RADAR_PhaseShift::RADAR_PhaseShift()
	: Input()
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
	, outBusSize_(0)
	, inputStartTime_(0.0)
	, inputTimeStep_(0.0)
	, inputFc_(0.0)
{
}

bool RADAR_PhaseShift::validateAndPrepare_()
{
	if (NumOfAntx < 1) NumOfAntx = 1;
	if (NumOfAnty < 1) NumOfAnty = 1;

	nAnt_ = NumOfAntx * NumOfAnty;
	if (nAnt_ < 1) nAnt_ = 1;

	phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);

	inputStartTime_ = Input.GetStartTime();
	inputTimeStep_ = Input.GetTimeStep();
	inputFc_ = Input.GetCharacterizationFrequency();

	outBusSize_ = static_cast<int>(output.GetSize());

	return true;
}

void RADAR_PhaseShift::applyOutputTiming_(double startTime)
{
	const size_t busSize = output.GetSize();
	for (size_t k = 0; k < busSize; ++k)
	{
		output[k].SetStartTime(startTime);
		if (inputTimeStep_ > 0.0) {
			output[k].SetTimeStep(inputTimeStep_);
		}
	}
}

void RADAR_PhaseShift::applyOutputFc_()
{
	const size_t busSize = output.GetSize();
	for (size_t k = 0; k < busSize; ++k)
	{
		output[k].SetCharacterizationFrequency(inputFc_);
	}
}

ERESULT RADAR_PhaseShift::CalculateLatency()
{
	inputStartTime_ = Input.GetStartTime();
	inputTimeStep_ = Input.GetTimeStep();
	applyOutputTiming_(inputStartTime_);
	return (ERESULT)0;
}

ERESULT RADAR_PhaseShift::PropagateCharacterizationFrequency()
{
	inputFc_ = Input.GetCharacterizationFrequency();
	applyOutputFc_();
	return (ERESULT)0;
}

bool RADAR_PhaseShift::Setup()
{
	validateAndPrepare_();

	Input.SetRate(1u);

	if (InTheta.IsConnected()) {
		InTheta.SetRate(1u);
	}
	if (InPhi.IsConnected()) {
		InPhi.SetRate(1u);
	}

	AntPhase.SetRate(static_cast<unsigned>(nAnt_));

	buildPhaseTable_();

	applyOutputTiming_(inputStartTime_);
	applyOutputFc_();

	return true;
}

bool RADAR_PhaseShift::Run()
{
	// 驱动 timed 模型时间轴，写法参考已有可编译的 TimedDFModel 模块。
	(void)Input.GetTime(0, GetCount());

	inputStartTime_ = Input.GetStartTime();
	inputTimeStep_ = Input.GetTimeStep();
	inputFc_ = Input.GetCharacterizationFrequency();

	// 如果 InTheta / InPhi 是可选动态输入，每次 firing 都要重新计算相位。
	buildPhaseTable_();

	applyOutputTiming_(inputStartTime_);
	applyOutputFc_();

	// EnvelopeSignal 本身不保存 Fc，Fc 在 EnvelopeCircularBuffer 上；
	// 这里只取复包络数值。
	const EnvSig x = Input[0];
	const Cx xCx = x.complex();

	const size_t busSize = output.GetSize();
	const size_t nWrite = std::min(busSize, static_cast<size_t>(nAnt_));

	// AntPhase 是 real 输出，帮助文档要求输出 NumOfAntx*NumOfAnty 个 token。
	// 它不应受 output bus 实际连接 lane 数影响。
	for (int k = 0; k < nAnt_; ++k)
	{
		AntPhase[k] = phaseCacheRad_[static_cast<size_t>(k)];
	}

	for (size_t k = 0; k < nWrite; ++k)
	{
		const double phase = phaseCacheRad_[k];
		const Cx yCx = xCx * phaseRotator_(phase);

		EnvSig y;
		y = yCx;

		output[k][0] = y;
	}

	// 多余输出 lane 清零，避免未写入 lane 残留旧值。
	for (size_t k = nWrite; k < busSize; ++k)
	{
		EnvSig y;
		y = Cx(0.0, 0.0);
		output[k][0] = y;
	}

	return true;
}

void RADAR_PhaseShift::buildPhaseTable_()
{
	if (nAnt_ <= 0) {
		nAnt_ = 1;
	}

	if (phaseCacheRad_.size() != static_cast<size_t>(nAnt_)) {
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

double RADAR_PhaseShift::getThetaRad_()
{
	// 帮助文档：可选输入端口 InTheta 的单位为 radians。
	if (InTheta.IsConnected()) {
		return InTheta[0];
	}

	// 注意：Theta 在界面中显示为 degrees，但注册参数时设置了 Units::ANGLE。
	// SystemVue 会把角度参数转换成内部弧度值传给 C++ 成员变量。
	// 因此这里不能再 deg2rad(Theta)，否则会发生二次转换。
	return Theta;
}

double RADAR_PhaseShift::getPhiRad_()
{
	// 帮助文档：可选输入端口 InPhi 的单位为 radians。
	if (InPhi.IsConnected()) {
		return InPhi[0];
	}

	// Phi 同 Theta，C++ 成员变量中已经是 radians。
	return Phi;
}

double RADAR_PhaseShift::computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const
{
	// 帮助文档公式：
	// theta(kx,ky) = (kx*dx/lambda)*sin(theta)*cos(phi)
	//              + (ky*dy/lambda)*sin(theta)*sin(phi)
	//
	// 参数 Dx / Dy 的单位已经是 wavelengths，AntPhase 输出单位为 radians，
	// 因此这里乘 2*pi。
	const double sx = std::sin(thetaRad) * std::cos(phiRad);
	const double sy = std::sin(thetaRad) * std::sin(phiRad);

	const double cycles = static_cast<double>(kx) * Dx * sx
		+ static_cast<double>(ky) * Dy * sy;

	return 2.0 * M_PI * cycles;
}

double RADAR_PhaseShift::getDesiredPhaseRad_(int index) const
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

RADAR_PhaseShift::Cx RADAR_PhaseShift::phaseRotator_(double phaseRad) const
{
	return Cx(std::cos(phaseRad), std::sin(phaseRad));
}

double RADAR_PhaseShift::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_PhaseShift::clampFinite(double x, double fallback)
{
	if (x != x) {
		return fallback;
	}

	if (!std::isfinite(x)) {
		return fallback;
	}

	return x;
}
