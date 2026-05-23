#include "Amplifier.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Amplifier)
{
	SET_MODEL_DESCRIPTION("Nonlinear Amplifier with Noise Figure");
	SET_MODEL_SYMBOL("SYM_Amplifier");
	SET_MODEL_CATEGORY("Analog/RF");

	// ===== 端口 =====
	{ auto p = ADD_MODEL_INPUT(input);
	p.SetDescription("input signal"); }

	{ auto p = ADD_MODEL_INPUT(control);
	p.SetDescription("gain control");
	p.SetOptional(); }

	{ auto p = ADD_MODEL_OUTPUT(output);
	p.SetDescription("output signal"); }

	// ===== GainUnit =====
	{ auto p = ADD_MODEL_ENUM_PARAM(GainUnit, GainUnitEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("voltage", GainUnitEnum::voltage);
	p.AddEnumeration("dB", GainUnitEnum::dB);
	p.SetDefaultValue("voltage");
	p.SetDescription("Gain unit for the Gain parameter or the optional voltage controlled gain input: voltage, dB"); }

	// ===== Gain =====
	{ auto p = ADD_MODEL_PARAM(Gain);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("1");
	p.SetDescription("Gain with unit defined by GainUnit (used if optional control input input is not used)"); }

	// ===== Quantization =====
	{ auto p = ADD_MODEL_ENUM_PARAM(Quantization, QuantizationEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("NO", QuantizationEnum::NO);
	p.AddEnumeration("Number of Bits (Uniform)", QuantizationEnum::Number_of_Bits_Uniform);
	p.AddEnumeration("Custom Levels", QuantizationEnum::Custom_Levels);
	p.SetDefaultValue("NO");
	p.SetDescription("Quantize Gain value: NO, Number of Bits (Uniform), Custom Levels"); }

	{ auto p = ADD_MODEL_PARAM(NumBits);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("6");
	p.SetDescription("Number of bits for quantization");
	p.SetHideCondition("Quantization ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(StepSize);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("0.5");
	p.SetDescription("Step size (LSB) for quantization");
	p.SetHideCondition("Quantization ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(Levels);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("[0, 0, 0]");
	p.SetDescription("Quantization Levels");
	p.SetHideCondition("Quantization ~= 2"); }

	{ auto p = ADD_MODEL_PARAM(MaxGain);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("10");
	p.SetDescription("Maximum gain level for quantization");
	p.SetHideCondition("Quantization ~= 1"); }

	// ===== GainError =====
	{ auto p = ADD_MODEL_ENUM_PARAM(GainError, GainErrorEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("None", GainErrorEnum::None);
	p.AddEnumeration("Normal", GainErrorEnum::Normal);
	p.AddEnumeration("Uniform", GainErrorEnum::Uniform);
	p.AddEnumeration("Custom Error", GainErrorEnum::Custom_Error);
	p.SetDefaultValue("None");
	p.SetDescription("Error distribution for Gain value with the distribution in GainUnit scale: None, Normal, Uniform, Custom Error"); }

	{ auto p = ADD_MODEL_PARAM(CustomError);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("0.0");
	p.SetDescription("Gain error");
	p.SetHideCondition("GainError ~= 3"); }

	{ auto p = ADD_MODEL_PARAM(StdDev);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("0.5");
	p.SetDescription("Standard deviation of normal distribution for GainError");
	p.SetHideCondition("GainError ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(Min);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("-0.5");
	p.SetDescription("Minimum GainError of uniform distribution");
	p.SetHideCondition("GainError ~= 2"); }

	{ auto p = ADD_MODEL_PARAM(Max);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("0.5");
	p.SetDescription("Maximum GainError of uniform distribution");
	p.SetHideCondition("GainError ~= 2"); }

	// ===== NoiseFigure =====
	{ auto p = ADD_MODEL_PARAM(NoiseFigure);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("0");
	p.SetDescription("Input noise figure in dB"); }

	// ===== GCType =====
	{ auto p = ADD_MODEL_ENUM_PARAM(GCType, GCTypeEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("none", GCTypeEnum::none);
	p.AddEnumeration("TOI", GCTypeEnum::TOI);
	p.AddEnumeration("dBc1", GCTypeEnum::dBc1);
	p.AddEnumeration("TOI+dBc1", GCTypeEnum::TOI_dBc1);
	p.AddEnumeration("PSat+GCSat+TOI", GCTypeEnum::PSat_GCSat_TOI);
	p.AddEnumeration("PSat+GCSat+dBc1", GCTypeEnum::PSat_GCSat_dBc1);
	p.AddEnumeration("PSat+GCSat+TOI+dBc1", GCTypeEnum::PSat_GCSat_TOI_dBc1);
	p.AddEnumeration("RappNonlinearity", GCTypeEnum::RappNonlinearity);
	p.AddEnumeration("Gain compression vs input power", GCTypeEnum::Gain_compression_vs_input_power);
	p.AddEnumeration("AM/AM and AM/PM vs input power", GCTypeEnum::AM_AM_and_AMPM_vs_input_power);
	p.SetDefaultValue("none");
	p.SetDescription("Gain compression type: none, TOI, dBc1, TOI+dBc1, PSat+GCSat+TOI, PSat+GCSat+dBc1, PSat+GCSat+TOI+dBc1, RappNonlinearity, Gain compression vs input power, AM/AM and AM/PM vs input power"); }

	{ auto p = ADD_MODEL_PARAM(TOIout);
	p.SetUnit(SystemVueModelBuilder::Units::POWER);
	p.SetDefaultValue("0.1");
	p.SetDescription("Output third order intercept power");
	p.SetHideCondition("GCType ~= 1 && GCType ~= 3 && GCType ~= 4 && GCType ~= 6"); }

	{ auto p = ADD_MODEL_PARAM(dBc1out);
	p.SetUnit(SystemVueModelBuilder::Units::POWER);
	p.SetDefaultValue("0.01");
	p.SetDescription("Output 1 dB gain compression power");
	p.SetHideCondition("GCType ~= 2 && GCType ~= 3 && GCType ~= 5 && GCType ~= 6"); }

	{ auto p = ADD_MODEL_PARAM(PSat);
	p.SetUnit(SystemVueModelBuilder::Units::POWER);
	p.SetDefaultValue("0.032");
	p.SetDescription("Saturation power");
	p.SetHideCondition("GCType ~= 4 && GCType ~= 5 && GCType ~= 6 && GCType ~= 7"); }

	{ auto p = ADD_MODEL_PARAM(GCSat);
	p.SetUnit(SystemVueModelBuilder::Units::POWER); // dB 临时用 W 表示
	p.SetDefaultValue("3");
	p.SetDescription("Gain compression at saturation in dB");
	p.SetHideCondition("GCType ~= 4 && GCType ~= 5 && GCType ~= 6"); }

	{ auto p = ADD_MODEL_PARAM(RappS);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("3");
	p.SetDescription("Rapp nonlinearity smoothness factor");
	p.SetHideCondition("GCType ~= 7"); }

	{ auto p = ADD_MODEL_PARAM(GComp);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("[0, 0, 0]");
	p.SetDescription("Array of triple values for Input Power(dBm) and either Gain(dB)/Phase(deg) change from small signal or AM-to-AM(dB/dB)/AM-to-PM(deg/dB)");
	p.SetHideCondition("GCType ~= 8 && GCType ~= 9"); }

	// ===== RefR =====
	{ auto p = ADD_MODEL_PARAM(RefR);
	p.SetUnit(SystemVueModelBuilder::Units::RESISTANCE);
	p.SetDefaultValue("50");
	p.SetDescription("Reference resistance"); }

	return true;
}
#endif

Amplifier::Amplifier(): GainUnit(GainUnitEnum::voltage),
	 Gain(1.0),
	 Quantization(QuantizationEnum::NO),
	 NumBits(6),
	 StepSize(0.5),
	 MaxGain(10.0),
	 GainError(GainErrorEnum::None),
	 StdDev(0.5),
	 Min(-0.5),
	 Max(0.5),
	 CustomError(0.0),
	 NoiseFigure(0.0),
	 GCType(GCTypeEnum::none),
	 TOIout(0.1),
	 dBc1out(0.01),
	 PSat(0.032),
	 GCSat(3.0),
	 RappS(3),
	 RefR(50.0)
{
	Levels.Resize(1, 3);
	Levels(0) = 0.0;
	Levels(1) = 0.0;
	Levels(2) = 0.0;

	GComp.Resize(1, 3);
	GComp(0) = 0.0;
	GComp(1) = 0.0;
	GComp(2) = 0.0;
}

ERESULT Amplifier::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	const double fc = input.GetCharacterizationFrequency();
	if (fc >= 0.0) {
		output.SetCharacterizationFrequency(fc);
	}
	else {
		POST_ERROR("characterization frequency must be >= 0.");
		bStatus = false;
	}

	return bStatus;
}

bool Amplifier::Setup()
{
//	(void)PropagateCharacterizationFrequency();
	output.SetRate(1U);

	// ===== GainError：内置黑盒结果为“每次运行抽一次，同次运行固定” =====
	gainErrOnce_ = 0.0;

	const unsigned long seedGE = 0;

	if (GainError == GainErrorEnum::Normal) {
		const double var = std::fabs(StdDev) * std::fabs(StdDev);
		rngGainErrN_.Initialize(0.0, var, seedGE);
		gainErrOnce_ = rngGainErrN_();
	}
	else if (GainError == GainErrorEnum::Uniform) {
		const double a = std::min(Min, Max);
		const double b = std::max(Min, Max);
		rngGainErrU_.Initialize(a, b, seedGE);
		gainErrOnce_ = rngGainErrU_();
	}
	else if (GainError == GainErrorEnum::Custom_Error) {
		gainErrOnce_ = CustomError;
	}

	// ===== NoiseFigure：Run 阶段首次拿到有效 SampleRate 后再初始化 =====
	noisePrepared_ = false;
	noiseEnabled_ = false;
	noiseSigma_ = 0.0;

	// ===== TOI+dBc1 参数合法性检查 =====
	if (GCType == GCTypeEnum::TOI_dBc1 && TOIout > 0.0 && dBc1out > 0.0) {
		const double fc = input.GetCharacterizationFrequency();
		const double offset = (fc > 0.0) ? 10.6357 : 11.8851;

		const double toiDbm = wattToDbm(TOIout);
		const double p1Dbm = wattToDbm(dBc1out);
		const double minToiDbm = p1Dbm + offset;

		if (toiDbm <= minToiDbm) {
            LOG_ERROR("TOIout of Amplifier: TOIout must be greater than dBc1out plus the required dB offset for TOI+dBc1 mode.");
			return false;
		}
	}

	// ===== GComp 表准备 =====
	gcompTable_ = TableData();
	amamTable_ = TableData();

	if (GCType == GCTypeEnum::Gain_compression_vs_input_power) {
		if (!prepareGCompTable()) return false;
	}
	else if (GCType == GCTypeEnum::AM_AM_and_AMPM_vs_input_power) {
		if (!prepareAMAMTable()) return false;
	}

	return true;
}

// ============================================================================
// 基础换算
// ============================================================================

double Amplifier::dbToLin(double db)
{
	return std::pow(10.0, db / 20.0);
}

double Amplifier::linToDb(double lin)
{
	if (lin <= 0.0) lin = 1e-300;
	return 20.0 * std::log10(lin);
}

double Amplifier::wattToDbm(double w)
{
	if (w <= 0.0) w = 1e-300;
	return 10.0 * std::log10(w) + 30.0;
}

double Amplifier::dbmToWatt(double dbm)
{
	return std::pow(10.0, (dbm - 30.0) / 10.0);
}

double Amplifier::wattToPeakVoltage(double w, double r)
{
	if (w <= 0.0 || r <= 0.0) return 0.0;
	return std::sqrt(2.0 * r * w);
}

double Amplifier::peakVoltageToWatt(double v, double r)
{
	if (r <= 0.0) return 0.0;
	return (v * v) / (2.0 * r);
}

double Amplifier::peakVoltageToDbm(double v, double r)
{
	return wattToDbm(peakVoltageToWatt(v, r));
}

double Amplifier::dbmToPeakVoltage(double dbm, double r)
{
	return wattToPeakVoltage(dbmToWatt(dbm), r);
}

// ============================================================================
// 增益、量化、误差
// ============================================================================

double Amplifier::quantizeGainDb(double gainDb) const
{
	if (Quantization == QuantizationEnum::NO) {
		return gainDb;
	}

	if (Quantization == QuantizationEnum::Number_of_Bits_Uniform) {
		if (NumBits <= 0) return gainDb;

		int levelsCount = 1;
		for (int i = 0; i < NumBits; ++i) {
			levelsCount *= 2;
		}

		const double step = (std::fabs(StepSize) < 1e-300) ? 1e-300 : std::fabs(StepSize);

		double best = MaxGain;
		double bestDiff = std::fabs(gainDb - best);

		for (int k = 1; k < levelsCount; ++k) {
			const double level = MaxGain - double(k) * step;
			const double diff = std::fabs(gainDb - level);

			// 中点向上：diff 相同取更大的 dB 电平
			if (diff < bestDiff || (std::fabs(diff - bestDiff) < 1e-14 && level > best)) {
				best = level;
				bestDiff = diff;
			}
		}

		return best;
	}

	if (Quantization == QuantizationEnum::Custom_Levels) {
		const int n = static_cast<int>(Levels.NumElements());
		if (n <= 0) return gainDb;

		double best = Levels(0);
		double bestDiff = std::fabs(gainDb - best);

		for (int i = 1; i < n; ++i) {
			const double level = Levels(i);
			const double diff = std::fabs(gainDb - level);

			// 中点向上：diff 相同取更大的 dB 电平
			if (diff < bestDiff || (std::fabs(diff - bestDiff) < 1e-14 && level > best)) {
				best = level;
				bestDiff = diff;
			}
		}

		return best;
	}

	return gainDb;
}

double Amplifier::computeSmallSignalGainLin(double gainSrc, double& outGainDb)
{
	double gainDb = 0.0;

	if (GainUnit == GainUnitEnum::dB) {
		gainDb = gainSrc;
	}
	else {
		double g = gainSrc;
		if (g <= 0.0) g = 1e-300;
		gainDb = linToDb(g);
	}

	// 量化统一在 dB 域
	gainDb = quantizeGainDb(gainDb);

	// GainError 按 GainUnit scale 叠加
	if (GainError != GainErrorEnum::None) {
		if (GainUnit == GainUnitEnum::dB) {
			gainDb += gainErrOnce_;
		}
		else {
			double gLin = dbToLin(gainDb);
			gLin += gainErrOnce_;
			if (gLin <= 0.0) gLin = 1e-300;
			gainDb = linToDb(gLin);
		}
	}

	outGainDb = gainDb;
	return dbToLin(gainDb);
}

// ============================================================================
// NoiseFigure
// ============================================================================

bool Amplifier::updateNoiseSigmaIfNeeded(double fc)
{
	if (noisePrepared_) {
		return true;
	}

	noisePrepared_ = true;
	noiseEnabled_ = false;
	noiseSigma_ = 0.0;

	if (NoiseFigure <= 0.0 || RefR <= 0.0) {
		return true;
	}

	double sr = input.GetSampleRate();
	if (sr <= 0.0) {
		const double ts = input.GetTimeStep();
		if (ts > 0.0) {
			sr = 1.0 / ts;
		}
	}

	if (sr <= 0.0) {
		POST_WARNING("Amplifier NoiseFigure: input sample rate is not available, noise is disabled.");
		return true;
	}

	const double factor = (fc > 0.0) ? 1.0 : 2.0;
	const double nfLin = std::pow(10.0, NoiseFigure / 10.0);

	if (nfLin <= 1.0) {
		return true;
	}

	const double vnRMS =
		std::sqrt(kBoltz * kT0 * (nfLin - 1.0) * (sr / factor) * RefR);

	if (vnRMS <= 0.0) {
		return true;
	}

	// 黑盒补测确认：复包络时 I/Q 每路 sigma 都约等于 vnRMS，不除 sqrt(2)
	noiseSigma_ = vnRMS;
	noiseEnabled_ = true;

	const double var = noiseSigma_ * noiseSigma_;

	rngNoiseI_.Initialize(0.0, var, 0);
	rngNoiseQ_.Initialize(0.0, var, 1);

	return true;
}

SystemVueModelBuilder::EnvelopeSignal Amplifier::addInputNoise(
	const SystemVueModelBuilder::EnvelopeSignal& xin,
	double fc)
{
	if (!noiseEnabled_) {
		return xin;
	}

	if (fc > 0.0) {
		const std::complex<double> x = xin.complex();
		const double ni = rngNoiseI_();
		const double nq = rngNoiseQ_();

		return SystemVueModelBuilder::EnvelopeSignal(x + std::complex<double>(ni, nq));
	}

	const double x = xin.real();
	const double n = rngNoiseI_();

	return SystemVueModelBuilder::EnvelopeSignal(x + n);
}

// ============================================================================
// GComp / AMAM 表格
// ============================================================================

bool Amplifier::parseGCompTriples(std::vector<double>& a,
	std::vector<double>& b,
	std::vector<double>& c) const
{
	a.clear();
	b.clear();
	c.clear();

	const int nElem = static_cast<int>(GComp.NumElements());
	if (nElem % 3 != 0 || nElem < 9) {
		//POST_ERROR("GComp of Amplifier: GComp must have at least 3 sets of triple values.");
		return false;
	}

	const int n = nElem / 3;
	a.resize(n);
	b.resize(n);
	c.resize(n);

	for (int i = 0; i < n; ++i) {
		a[i] = GComp(3 * i + 0);
		b[i] = GComp(3 * i + 1);
		c[i] = GComp(3 * i + 2);
	}

	for (int i = 1; i < n; ++i) {
		if (a[i] <= a[i - 1]) {
			//POST_ERROR("GComp of Amplifier: input power values must be strictly increasing.");
			return false;
		}
	}

	return true;
}

bool Amplifier::prepareGCompTable()
{
	std::vector<double> pin;
	std::vector<double> gc;
	std::vector<double> pc;

	if (!parseGCompTriples(pin, gc, pc)) {
		return false;
	}

	gcompTable_.pinDbm = pin;
	gcompTable_.gcDb = gc;
	gcompTable_.pcDeg = pc;
	gcompTable_.valid = true;
	gcompTable_.highLinearExtension = false;

	// Gain compression vs input power 单独使用半端点斜率规则；
	// 该规则来自 10A/10B 黑盒结果：AM/AM 模式不能启用，否则会破坏 11B。
	gcompTable_.halfLastSlope = true;

	// 第一端点修正：只有首点 GC 已经不是 0 时才启用。
	// 当前 10B 黑盒结果显示，直接用 sec[0] 会偏高，0.5*sec[0] 又偏低；
	// 更接近内置的是把首端点斜率设成 0.5*(sec[0]+sec[1])。
	// 10A/10C 首点 GC=0 时保持旧结果。
	gcompTable_.averageFirstSlope = (std::fabs(gc.front()) > 1e-12);

	return true;
}

bool Amplifier::prepareAMAMTable()
{
	std::vector<double> pin;
	std::vector<double> am;
	std::vector<double> pm;

	if (!parseGCompTriples(pin, am, pm)) {
		return false;
	}

	const int n = static_cast<int>(pin.size());

	if (am[0] < 0.99 || am[0] > 1.0 || pm[0] < -0.1 || pm[0] > 0.1) {
		POST_WARNING("GComp of Amplifier: For proper modeling of AM2AM and AM2PM data, the first input power point should be in the linear region, that is, the AM2AM and AM2PM data for the first point should be in the range [0.99, 1] and [-0.1, 0.1] respectively.");
	}

	std::vector<double> dPin(n - 1, 0.0);
	std::vector<double> dPout(n - 1, 0.0);
	std::vector<double> dPhase(n - 1, 0.0);

	for (int i = 0; i < n - 1; ++i) {
		dPin[i] = pin[i + 1] - pin[i];
	}

	dPout[0] = am[0] * dPin[0];
	dPhase[0] = pm[0] * dPin[0];

	for (int i = 1; i < n - 1; ++i) {
		dPout[i] = (2.0 * am[i] - dPout[i - 1] / dPin[i - 1]) * dPin[i];
		dPhase[i] = (2.0 * pm[i] - dPhase[i - 1] / dPin[i - 1]) * dPin[i];
	}

	std::vector<double> gc(n, 0.0);
	std::vector<double> pc(n, 0.0);

	gc[0] = 0.0;
	pc[0] = 0.0;

	for (int i = 1; i < n; ++i) {
		gc[i] = gc[i - 1] + dPout[i - 1] - dPin[i - 1];
		pc[i] = pc[i - 1] + dPhase[i - 1];
	}

	bool allLinear = true;
	for (int i = 0; i < n; ++i) {
		if (std::fabs(gc[i]) > 1e-12) {
			allLinear = false;
			break;
		}
	}

	amamTable_.pinDbm = pin;
	amamTable_.gcDb = gc;
	amamTable_.pcDeg = pc;
	amamTable_.valid = true;

	// 黑盒验证：AM/AM 全线性表格在高功率端继续线性；
	// 非线性 AM/AM 表格高功率端按最后 Pout 固定。
	amamTable_.highLinearExtension = allLinear;

	// AM/AM 模式保持常规端点斜率，否则 11B 会被破坏。
	amamTable_.halfLastSlope = false;
	amamTable_.averageFirstSlope = false;

	return true;
}

double Amplifier::tableOutputAmplitude(double ain,
	double c1,
	double gainDb,
	const TableData& table) const
{
	if (!table.valid || table.pinDbm.size() < 3 || RefR <= 0.0) {
		return c1 * ain;
	}

	if (ain <= 0.0) {
		return 0.0;
	}

	const int n = static_cast<int>(table.pinDbm.size());

	// 当前输入幅度换算为输入功率 dBm
	const double pinNow = peakVoltageToDbm(ain, RefR);

	std::vector<double> x(n, 0.0); // Pin_dBm
	std::vector<double> y(n, 0.0); // Pout_dBm

	for (int i = 0; i < n; ++i) {
		x[i] = table.pinDbm[i];

		// Pout = Pin + small-signal gain + gain compression
		y[i] = table.pinDbm[i] + gainDb + table.gcDb[i];
	}

	// 低于第一个表格点：按小信号线性输出
	if (pinNow < x.front()) {
		return c1 * ain;
	}

	// 高于最后一个表格点
	if (pinNow >= x.back()) {
		if (table.highLinearExtension) {
			return c1 * ain;
		}

		// 非全线性表格：固定最后一个 Pout
		return dbmToPeakVoltage(y.back(), RefR);
	}

	// ===== 计算每一段割线斜率：dPout/dPin =====
	std::vector<double> sec(n - 1, 0.0);

	for (int i = 0; i < n - 1; ++i) {
		const double dx = x[i + 1] - x[i];

		if (std::fabs(dx) < 1e-300) {
			sec[i] = 0.0;
		}
		else {
			sec[i] = (y[i + 1] - y[i]) / dx;
		}
	}

	// ===== 节点斜率：局部三次 Hermite / Catmull-Rom 风格 =====
	std::vector<double> m(n, 0.0);

	// Gain compression vs input power：
	// 如果首点 GC 已经非 0，10B 的黑盒结果显示首端点斜率更接近
	// 相邻两段割线斜率的平均值，而不是 sec[0] 或 0.5*sec[0]。
	// n>=3 时一定存在 sec[1]，因为 GComp 至少 3 组三元组。
	if (table.averageFirstSlope && sec.size() >= 2) {
		m[0] = 0.5 * (sec[0] + sec[1]);
	}
	else {
		m[0] = sec[0];
	}

	// Gain compression vs input power：最后一个端点斜率按 0.5 * 最后一段割线处理；
	// AM/AM 模式：保持 m[n-1] = sec[n-2]，这是 11B 已经基本对齐的关键。
	if (table.halfLastSlope) {
		m[n - 1] = 0.5 * sec[n - 2];
	}
	else {
		m[n - 1] = sec[n - 2];
	}

	for (int i = 1; i < n - 1; ++i) {
		m[i] = 0.5 * (sec[i - 1] + sec[i]);
	}

	// ===== 找到当前 Pin 所在区间 =====
	int k = 0;

	for (int i = 0; i < n - 1; ++i) {
		if (pinNow >= x[i] && pinNow <= x[i + 1]) {
			k = i;
			break;
		}
	}

	const double x0 = x[k];
	const double x1 = x[k + 1];
	const double y0 = y[k];
	const double y1 = y[k + 1];

	const double h = x1 - x0;

	if (std::fabs(h) < 1e-300) {
		return dbmToPeakVoltage(y0, RefR);
	}

	const double t = (pinNow - x0) / h;

	const double h00 = 2.0 * t * t * t - 3.0 * t * t + 1.0;
	const double h10 = t * t * t - 2.0 * t * t + t;
	const double h01 = -2.0 * t * t * t + 3.0 * t * t;
	const double h11 = t * t * t - t * t;

	double poutNow =
		h00 * y0 +
		h10 * h * m[k] +
		h01 * y1 +
		h11 * h * m[k + 1];

	return dbmToPeakVoltage(poutNow, RefR);
}

// ============================================================================
// 多项式工具
// ============================================================================

double Amplifier::evalOddPolynomial(double x, const std::vector<double>& coeff)
{
	double y = 0.0;
	double xp = x;

	for (size_t i = 0; i < coeff.size(); ++i) {
		y += coeff[i] * xp;
		xp *= x * x;
	}

	return y;
}

double Amplifier::evalOddDerivative(double x, const std::vector<double>& coeff)
{
	double y = 0.0;
	double xp = 1.0;

	for (size_t i = 0; i < coeff.size(); ++i) {
		const double order = 2.0 * double(i) + 1.0;
		y += order * coeff[i] * xp;
		xp *= x * x;
	}

	return y;
}

double Amplifier::findFirstPeakX(const std::vector<double>& coeff, double hint)
{
	if (coeff.empty()) {
		return std::numeric_limits<double>::infinity();
	}

	double hi = (hint > 0.0) ? hint : 1.0;
	double dhi = evalOddDerivative(hi, coeff);

	int expand = 0;
	while (dhi > 0.0 && expand < 80) {
		hi *= 2.0;
		dhi = evalOddDerivative(hi, coeff);
		++expand;
	}

	if (dhi > 0.0) {
		return std::numeric_limits<double>::infinity();
	}

	double lo = 0.0;
	for (int i = 0; i < 100; ++i) {
		const double mid = 0.5 * (lo + hi);
		const double dm = evalOddDerivative(mid, coeff);

		if (dm > 0.0) {
			lo = mid;
		}
		else {
			hi = mid;
		}
	}

	return 0.5 * (lo + hi);
}

bool Amplifier::solve4x4(double a[4][4], double b[4], double x[4])
{
	const int n = 4;

	for (int i = 0; i < n; ++i) {
		int pivot = i;
		double maxAbs = std::fabs(a[i][i]);

		for (int r = i + 1; r < n; ++r) {
			const double v = std::fabs(a[r][i]);
			if (v > maxAbs) {
				maxAbs = v;
				pivot = r;
			}
		}

		if (maxAbs < 1e-300) {
			return false;
		}

		if (pivot != i) {
			for (int c = i; c < n; ++c) {
				std::swap(a[i][c], a[pivot][c]);
			}
			std::swap(b[i], b[pivot]);
		}

		const double div = a[i][i];
		for (int c = i; c < n; ++c) {
			a[i][c] /= div;
		}
		b[i] /= div;

		for (int r = 0; r < n; ++r) {
			if (r == i) continue;

			const double f = a[r][i];
			for (int c = i; c < n; ++c) {
				a[r][c] -= f * a[i][c];
			}
			b[r] -= f * b[i];
		}
	}

	for (int i = 0; i < n; ++i) {
		x[i] = b[i];
	}

	return true;
}

// ============================================================================
// GCType 非线性
// ============================================================================

double Amplifier::applyTOI(double ain, double c1) const
{
	if (ain <= 0.0 || RefR <= 0.0 || TOIout <= 0.0) {
		return c1 * ain;
	}

	// 黑盒测试确认：c3 = -c1^3 / TOIv^2，不带 4/3
	const double toiV = wattToPeakVoltage(TOIout, RefR);
	const double c3 = -(c1 * c1 * c1) / (toiV * toiV);

	const double xmax = std::sqrt(-c1 / (3.0 * c3));
	const double ymax = c1 * xmax + c3 * xmax * xmax * xmax;

	if (ain >= xmax) {
		return ymax;
	}

	double y = c1 * ain + c3 * ain * ain * ain;
	if (y < 0.0) y = 0.0;
	return y;
}

double Amplifier::applydBc1(double ain, double c1) const
{
	if (ain <= 0.0 || RefR <= 0.0 || dBc1out <= 0.0) {
		return c1 * ain;
	}

	const double dBc1v = wattToPeakVoltage(dBc1out, RefR);
	const double y1 = dBc1v;
	const double x1 = (dBc1v / kOneDbVoltageRatio) / c1;

	const double c3 = (y1 - c1 * x1) / (x1 * x1 * x1);

	const double xmax = std::sqrt(-c1 / (3.0 * c3));
	const double ymax = c1 * xmax + c3 * xmax * xmax * xmax;

	if (ain >= xmax) {
		return ymax;
	}

	double y = c1 * ain + c3 * ain * ain * ain;
	if (y < 0.0) y = 0.0;
	return y;
}

double Amplifier::applyTOIdBc1(double ain, double c1) const
{
	if (ain <= 0.0 || RefR <= 0.0 || TOIout <= 0.0 || dBc1out <= 0.0) {
		return c1 * ain;
	}

	const double toiV = wattToPeakVoltage(TOIout, RefR);
	const double c3 = -(c1 * c1 * c1) / (toiV * toiV);

	const double dBc1v = wattToPeakVoltage(dBc1out, RefR);
	const double y1 = dBc1v;
	const double x1 = (dBc1v / kOneDbVoltageRatio) / c1;

	const double c5 = (y1 - c1 * x1 - c3 * std::pow(x1, 3.0)) / std::pow(x1, 5.0);

	std::vector<double> coeff;
	coeff.push_back(c1);
	coeff.push_back(c3);
	coeff.push_back(c5);

	const double xmax = findFirstPeakX(coeff, x1 * 2.0);
	if (std::isfinite(xmax)) {
		const double ymax = evalOddPolynomial(xmax, coeff);
		if (ain >= xmax) {
			return ymax;
		}
	}

	double y = evalOddPolynomial(ain, coeff);
	if (y < 0.0) y = 0.0;
	return y;
}

bool Amplifier::computePSatPolynomialCoeffs(double c1,
	double fc,
	std::vector<double>& coeff,
	double& xs,
	double& ys) const
{
	coeff.clear();
	xs = 0.0;
	ys = 0.0;

	if (RefR <= 0.0 || PSat <= 0.0 || c1 <= 0.0) {
		return false;
	}

	// 组合模式中缺少 TOI 或 dBc1 时，按帮助文档用 dB_offset 互推
	const double offset = (fc > 0.0) ? 10.6357 : 11.8851;

	double toiW = TOIout;
	double p1W = dBc1out;

	if (GCType == GCTypeEnum::PSat_GCSat_TOI) {
		const double toiDbm = wattToDbm(TOIout);
		const double p1Dbm = toiDbm - offset;
		p1W = dbmToWatt(p1Dbm);
	}
	else if (GCType == GCTypeEnum::PSat_GCSat_dBc1) {
		const double p1Dbm = wattToDbm(dBc1out);
		const double toiDbm = p1Dbm + offset;
		toiW = dbmToWatt(toiDbm);
	}

	if (toiW <= 0.0 || p1W <= 0.0) {
		return false;
	}

	const double toiV = wattToPeakVoltage(toiW, RefR);
	const double c3 = -(c1 * c1 * c1) / (toiV * toiV);

	const double p1V = wattToPeakVoltage(p1W, RefR);
	const double x1 = (p1V / kOneDbVoltageRatio) / c1;
	const double y1 = p1V;

	ys = wattToPeakVoltage(PSat, RefR);

	const double gcSatR = std::pow(10.0, GCSat / 20.0);
	xs = ys * gcSatR / c1;

	if (x1 <= 0.0 || xs <= 0.0 || ys <= 0.0) {
		return false;
	}

	double a[4][4] = {};
	double b[4] = {};
	double sol[4] = {};

	// unknown: c5, c7, c9, c11
	a[0][0] = std::pow(x1, 5.0);
	a[0][1] = std::pow(x1, 7.0);
	a[0][2] = std::pow(x1, 9.0);
	a[0][3] = std::pow(x1, 11.0);
	b[0] = y1 - c1 * x1 - c3 * std::pow(x1, 3.0);

	a[1][0] = std::pow(xs, 5.0);
	a[1][1] = std::pow(xs, 7.0);
	a[1][2] = std::pow(xs, 9.0);
	a[1][3] = std::pow(xs, 11.0);
	b[1] = ys - c1 * xs - c3 * std::pow(xs, 3.0);

	a[2][0] = 5.0  * std::pow(xs, 4.0);
	a[2][1] = 7.0  * std::pow(xs, 6.0);
	a[2][2] = 9.0  * std::pow(xs, 8.0);
	a[2][3] = 11.0 * std::pow(xs, 10.0);
	b[2] = -c1 - 3.0 * c3 * std::pow(xs, 2.0);

	a[3][0] = 20.0  * std::pow(xs, 3.0);
	a[3][1] = 42.0  * std::pow(xs, 5.0);
	a[3][2] = 72.0  * std::pow(xs, 7.0);
	a[3][3] = 110.0 * std::pow(xs, 9.0);
	b[3] = -6.0 * c3 * xs;

	if (!solve4x4(a, b, sol)) {
		return false;
	}

	coeff.push_back(c1);
	coeff.push_back(c3);
	coeff.push_back(sol[0]);
	coeff.push_back(sol[1]);
	coeff.push_back(sol[2]);
	coeff.push_back(sol[3]);

	return true;
}

double Amplifier::applyPSatGCSat(double ain, double c1, double fc) const
{
	if (ain <= 0.0) {
		return 0.0;
	}

	std::vector<double> coeff;
	double xs = 0.0;
	double ys = 0.0;

	if (!computePSatPolynomialCoeffs(c1, fc, coeff, xs, ys)) {
		return c1 * ain;
	}

	// 黑盒测试确认：高功率端钳位到 PSatv
	if (ain >= xs) {
		return ys;
	}

	double y = evalOddPolynomial(ain, coeff);

	if (y < 0.0) y = 0.0;
	if (y > ys)  y = ys;

	return y;
}

double Amplifier::applyRapp(double ain, double c1) const
{
	if (ain <= 0.0 || RefR <= 0.0 || PSat <= 0.0) {
		return c1 * ain;
	}

	const double psatV = wattToPeakVoltage(PSat, RefR);
	if (psatV <= 0.0) {
		return c1 * ain;
	}

	const double s = (RappS <= 0) ? 1.0 : double(RappS);
	const double ratio = (c1 * ain) / psatV;

	const double denom = std::pow(1.0 + std::pow(ratio, 2.0 * s), 1.0 / (2.0 * s));
	double y = (c1 * ain) / denom;

	if (y < 0.0) y = 0.0;
	return y;
}

// ============================================================================
// 输入输出封装
// ============================================================================

double Amplifier::getInputAmplitude(const SystemVueModelBuilder::EnvelopeSignal& xin,
	double fc) const
{
	if (fc > 0.0) {
		return std::abs(xin.complex());
	}

	return std::fabs(xin.real());
}

SystemVueModelBuilder::EnvelopeSignal Amplifier::makeOutputWithAmplitude(
	const SystemVueModelBuilder::EnvelopeSignal& xin,
	double fc,
	double aout) const
{
	if (aout < 0.0) aout = 0.0;

	if (fc > 0.0) {
		const std::complex<double> x = xin.complex();
		const double ain = std::abs(x);

		if (ain <= 0.0) {
			return SystemVueModelBuilder::EnvelopeSignal(std::complex<double>(0.0, 0.0));
		}

		return SystemVueModelBuilder::EnvelopeSignal(x * (aout / ain));
	}

	const double x = xin.real();
	const double sign = (x < 0.0) ? -1.0 : 1.0;

	return SystemVueModelBuilder::EnvelopeSignal(sign * aout);
}

// ============================================================================
// Run
// ============================================================================

bool Amplifier::Run()
{
	using SystemVueModelBuilder::EnvelopeSignal;

	// 强制驱动时间轴，保证 Run 阶段能拿到有效 SampleRate / TimeStep
	(void)input.GetTime(0, GetCount());

	const double fc = input.GetCharacterizationFrequency();

	if (!updateNoiseSigmaIfNeeded(fc)) {
		return false;
	}

	// ===== 1. 读取输入 =====
	EnvelopeSignal xin = input[0U];

	// ===== 2. 先加输入端噪声 =====
	EnvelopeSignal x2 = addInputNoise(xin, fc);

	// ===== 3. 获取增益源 =====
	double gainSrc = Gain;
	if (control.IsConnected()) {
		gainSrc = control[0U];
	}

	// ===== 4. 计算小信号增益 =====
	double gainDb = 0.0;
	const double c1 = computeSmallSignalGainLin(gainSrc, gainDb);

	// ===== 5. 按 GCType 处理 =====
	EnvelopeSignal yout;

	if (GCType == GCTypeEnum::none) {
		if (fc > 0.0) {
			yout = EnvelopeSignal(x2.complex() * c1);
		}
		else {
			yout = EnvelopeSignal(x2.real() * c1);
		}
	}
	else {
		const double ain = getInputAmplitude(x2, fc);
		double aout = c1 * ain;

		switch (GCType) {
		case GCTypeEnum::TOI:
			aout = applyTOI(ain, c1);
			break;

		case GCTypeEnum::dBc1:
			aout = applydBc1(ain, c1);
			break;

		case GCTypeEnum::TOI_dBc1:
			aout = applyTOIdBc1(ain, c1);
			break;

		case GCTypeEnum::PSat_GCSat_TOI:
		case GCTypeEnum::PSat_GCSat_dBc1:
		case GCTypeEnum::PSat_GCSat_TOI_dBc1:
			aout = applyPSatGCSat(ain, c1, fc);
			break;

		case GCTypeEnum::RappNonlinearity:
			aout = applyRapp(ain, c1);
			break;

		case GCTypeEnum::Gain_compression_vs_input_power:
			// 黑盒确认：PC 第三列不作用输出相位，只做幅度压缩
			aout = tableOutputAmplitude(ain, c1, gainDb, gcompTable_);
			break;

		case GCTypeEnum::AM_AM_and_AMPM_vs_input_power:
			// 黑盒确认：合法 AMPM 表中 AM2PM 不作用输出相位，只用 AM2AM 递推得到幅度压缩
			aout = tableOutputAmplitude(ain, c1, gainDb, amamTable_);
			break;

		default:
			aout = c1 * ain;
			break;
		}

		yout = makeOutputWithAmplitude(x2, fc, aout);
	}

	output[0U] = yout;
	return true;
}
