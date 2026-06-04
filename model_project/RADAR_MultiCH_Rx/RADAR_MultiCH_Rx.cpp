#include "RADAR_MultiCH_Rx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_MultiCH_Rx)
{
	SET_MODEL_DESCRIPTION("RADAR ideal multichannel receiver");
	SET_MODEL_CATEGORY("Array TR");

	// 端口
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetDescription("input signal");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("IQ output");
	}

	// 参数
	{
		auto p = ADD_MODEL_PARAM(RefFreq);
		p.SetDefaultValue("1000000");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("Internal reference frequency");
	}
	{
		auto p = ADD_MODEL_PARAM(NDensity);
		p.SetDefaultValue("-173.975");
		p.SetDescription("Noise spectral density at output, in dBm/Hz");
	}

	{
		auto p = ADD_MODEL_PARAM(Sensitivity);
		p.SetDefaultValue("[1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0;1.0]");
		p.SetDescription("Voltage output sensitivity, Vout/Vin");
	}
	{
		auto p = ADD_MODEL_PARAM(Phase);
		p.SetDefaultValue("[0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0]");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("Reference phase in degrees");
	}
	{
		auto p = ADD_MODEL_PARAM(IQGainImbalance);
		p.SetDefaultValue("[0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0]");
		p.SetDescription("The gain imbalance in dB, Q channel relative to I channel");
	}
	{
		auto p = ADD_MODEL_PARAM(IQPhaseImbalance);
		p.SetDefaultValue("[0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0]");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The phase imbalance in degrees, Q channel relative to I channel");
	}

	{
		auto p = ADD_MODEL_PARAM(NumOfCh);
		p.SetDefaultValue("16");
		p.SetDescription("The number of rx channel");
	}

	{
		auto p = ADD_MODEL_PARAM(ImbalanceCoef);
		p.SetDefaultValue("[1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,"
			"1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0]");
		p.SetDescription("The imbalance coefficient of channels");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_MultiCH_Rx::RADAR_MultiCH_Rx()
	: RefFreq(1000000.0)
	, NDensity(-173.975)
	, NumOfCh(16)
{
}

bool RADAR_MultiCH_Rx::buildCache_()
{
	nChExpected_ = std::max(0, NumOfCh);

	sens_.assign(static_cast<size_t>(nChExpected_), 1.0);
	phaseDeg_.assign(static_cast<size_t>(nChExpected_), 0.0);
	iqGainDb_.assign(static_cast<size_t>(nChExpected_), 0.0);
	iqPhaseDeg_.assign(static_cast<size_t>(nChExpected_), 0.0);
	imbCoef_.assign(static_cast<size_t>(nChExpected_), Cx(1.0, 0.0));

	auto needExactLen = [this](size_t got, const char*) -> bool {
		return got == static_cast<size_t>(nChExpected_);
	};

	if (!needExactLen(Sensitivity.NumElements(), "Sensitivity")) return false;
	if (!needExactLen(Phase.NumElements(), "Phase")) return false;
	if (!needExactLen(IQGainImbalance.NumElements(), "IQGainImbalance")) return false;
	if (!needExactLen(IQPhaseImbalance.NumElements(), "IQPhaseImbalance")) return false;
	if (!needExactLen(ImbalanceCoef.NumElements(), "ImbalanceCoef")) return false;

	for (int k = 0; k < nChExpected_; ++k)
	{
		sens_[k] = Sensitivity(k);
		phaseDeg_[k] = Phase(k);
		iqGainDb_[k] = IQGainImbalance(k);
		iqPhaseDeg_[k] = IQPhaseImbalance(k);
		imbCoef_[k] = ImbalanceCoef(k);
	}

	return (RefFreq > 0.0);
}

void RADAR_MultiCH_Rx::applyOutputTiming_()
{
	// 所有输出通道共用 lane0 timing
	const size_t bs = output.GetSize();
	if (inBusSize_ == 0) return;

	const double st = input[0].GetStartTime();
	double ts = input[0].GetTimeStep();
	double fs = input[0].GetSampleRate();
	if (fs <= 0.0 && ts > 0.0) fs = 1.0 / ts;
	if (ts <= 0.0 && fs > 0.0) ts = 1.0 / fs;

	for (size_t k = 0; k < bs; ++k)
	{
		output[k].SetStartTime(st);
		if (ts > 0.0) output[k].SetTimeStep(ts);
		if (fs > 0.0) output[k].SetSampleRate(fs);
	}
}

ERESULT RADAR_MultiCH_Rx::CalculateLatency()
{
	inBusSize_ = input.GetSize();
	outBusSize_ = output.GetSize();

	ts0_ = 0.0;
	fs0_ = 0.0;

	if (inBusSize_ > 0)
	{
		ts0_ = input[0].GetTimeStep();
		fs0_ = input[0].GetSampleRate();
		if (fs0_ <= 0.0 && ts0_ > 0.0) fs0_ = 1.0 / ts0_;
		if (ts0_ <= 0.0 && fs0_ > 0.0) ts0_ = 1.0 / fs0_;
	}

	applyOutputTiming_();
	return (ERESULT)0;
}

RADAR_MultiCH_Rx::Cx RADAR_MultiCH_Rx::applyIQImbalance_(const Cx& z, double gainDb, double phaseDeg)
{
	// wide-linear I/Q 失衡模型
	const double g = std::pow(10.0, gainDb / 20.0);
	const double ph = deg2rad(phaseDeg);

	const Cx ejph(std::cos(ph), std::sin(ph));   // e^{+jphi}
	const Cx ejm(std::cos(ph), -std::sin(ph));   // e^{-jphi}

	const Cx alpha = 0.5 * (Cx(1.0, 0.0) + g * ejph);
	const Cx beta = 0.5 * (Cx(1.0, 0.0) - g * ejm);

	return alpha * z + beta * std::conj(z);
}

RADAR_MultiCH_Rx::Cx RADAR_MultiCH_Rx::makeNoise_(double fs)
{
	if (!(fs > 0.0))
		return Cx(0.0, 0.0);

	const double psd_W_per_Hz = 1e-3 * std::pow(10.0, NDensity / 10.0);
	const double R = 50.0;

	const double kVarCal = 8;
	const double var = psd_W_per_Hz * R * fs * kVarCal;
	const double sigma = std::sqrt(std::max(0.0, var / 2.0));

	return Cx(sigma * randn_(), sigma * randn_());
}

bool RADAR_MultiCH_Rx::Setup()
{
	inBusSize_ = input.GetSize();
	outBusSize_ = output.GetSize();

	ts0_ = 0.0;
	fs0_ = 0.0;
	if (inBusSize_ > 0)
	{
		ts0_ = input[0].GetTimeStep();
		fs0_ = input[0].GetSampleRate();
		if (fs0_ <= 0.0 && ts0_ > 0.0) fs0_ = 1.0 / ts0_;
		if (ts0_ <= 0.0 && fs0_ > 0.0) ts0_ = 1.0 / fs0_;
	}

	applyOutputTiming_();

	if (!buildCache_()) return false;

	rngState_ = 1;
	haveSpare_ = false;
	spare_ = 0.0;

	sampleIndex_ = 0;
	return true;
}

bool RADAR_MultiCH_Rx::Run()
{
	inBusSize_ = input.GetSize();
	outBusSize_ = output.GetSize();
	if (inBusSize_ == 0 || outBusSize_ == 0) return true;

	// 驱动时间轴
	(void)input[0].GetTime(0, GetCount());

	// lane0 timing（全局 Ts/Fs）
	ts0_ = input[0].GetTimeStep();
	fs0_ = input[0].GetSampleRate();
	if (fs0_ <= 0.0 && ts0_ > 0.0) fs0_ = 1.0 / ts0_;
	if (ts0_ <= 0.0 && fs0_ > 0.0) ts0_ = 1.0 / fs0_;

	applyOutputTiming_();

	const size_t nWrite = std::min(outBusSize_, static_cast<size_t>(nChExpected_));
	if (nWrite == 0 || !(ts0_ > 0.0) || !(fs0_ > 0.0)) return true;

	// 采样编号与时间基准
	const long long n = static_cast<long long>(GetCount());
	const double st0 = input[0].GetStartTime();
	const double t0 = st0 + static_cast<double>(n) * ts0_;

	// ======================================================
	// Ref 的“离散相位累加器”推进：step = (N - 2)
	// ======================================================
	long long step = static_cast<long long>(nWrite) - 2LL;
	if (step < 0) step = 0;

	// ======================================================
	// 公共相位坡补偿
	// df_common = remainder((N-2)*RefFreq, Fs)
	// corr_common = exp(-j2π df_common t0)
	// ======================================================
	const double df_common = std::remainder(
		static_cast<double>(static_cast<long long>(nWrite) - 2LL) * RefFreq,
		fs0_);

	const double cyc_common = std::remainder(df_common * t0, 1.0);
	const double ang_common = kTwoPi * cyc_common;
	const Cx corr_common(std::cos(ang_common), -std::sin(ang_common)); // e^{-j2π df_common t0}

	// ======================================================
	// DEBUG MODE：
	// 0 = 正常输出 y
	// 1 = 输出 rot_fc
	// 2 = 输出 rot_ref
	// 3 = 输出 rot_fc * rot_ref
	// 4 = 输出 corr_common
	// ======================================================
#define RX_DEBUG_MODE 0

	for (size_t k = 0; k < nWrite; ++k)
	{
		// 读输入 envelope
		EnvSig env;
		env = std::complex<double>(0.0, 0.0);
		if (k < inBusSize_) env = input[k][0];
		Cx x = env.complex();

		// Fc 元数据（无则用 lane0）
		double fc_in = -1.0;
		if (k < inBusSize_) fc_in = input[k].GetCharacterizationFrequency();
		if (fc_in < 0.0)    fc_in = input[0].GetCharacterizationFrequency();

		// 默认设成 1，避免 fc 无效时未初始化
		Cx rot_fc(1.0, 0.0);
		Cx rot_ref(1.0, 0.0);

		if (fc_in >= 0.0)
		{
			// 交错采样的“真实取样时刻”
			const double t_fc = t0 + static_cast<double>(k) * ts0_;

			// Fc 折叠到 Fs
			const double f_fc = std::remainder(fc_in, fs0_);
			const double cyc_fc = std::remainder(f_fc * t_fc, 1.0);
			const double ang_fc = kTwoPi * cyc_fc;
			rot_fc = Cx(std::cos(ang_fc), std::sin(ang_fc)); // e^{+j2π f_fc t_fc}

			// RefFreq 用“离散相位累加器时刻”
			// acc = n*(N-2) + k
			const long long acc = n * step + static_cast<long long>(k);
			const double t_ref = st0 + static_cast<double>(acc) * ts0_;

			const double cyc_ref = std::remainder(RefFreq * t_ref, 1.0);
			const double ang_ref = kTwoPi * cyc_ref;
			rot_ref = Cx(std::cos(ang_ref), -std::sin(ang_ref)); // e^{-j2π Ref t_ref}

			// 合成：Fc -> Ref
			x *= (rot_fc * rot_ref);
		}

#if RX_DEBUG_MODE == 1
		output[k][0] = rot_fc;
		continue;
#elif RX_DEBUG_MODE == 2
		output[k][0] = rot_ref;
		continue;
#elif RX_DEBUG_MODE == 3
		output[k][0] = (rot_fc * rot_ref);
		continue;
#elif RX_DEBUG_MODE == 4
		output[k][0] = corr_common;
		continue;
#endif

		// Phase（+ph0）
		const double ph0 = deg2rad(phaseDeg_[static_cast<int>(k)]);
		const Cx ejph(std::cos(ph0), std::sin(ph0));
		Cx y = x * ejph;

		// IQ 失衡 + 通道系数 + 灵敏度 + 噪声
		y = applyIQImbalance_(y, iqGainDb_[static_cast<int>(k)], iqPhaseDeg_[static_cast<int>(k)]);
		y *= imbCoef_[static_cast<int>(k)];
		y *= sens_[static_cast<int>(k)];
		y += makeNoise_(fs0_);

		// 公共相位坡补偿
		y *= corr_common;

		output[k][0] = y;
	}

#undef RX_DEBUG_MODE

	for (size_t k = nWrite; k < outBusSize_; ++k)
		output[k][0] = Cx(0.0, 0.0);

	return true;
}