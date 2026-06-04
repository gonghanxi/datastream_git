#include "RADAR_Tx_DBS_2D.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Tx_DBS_2D)
{
	SET_MODEL_DESCRIPTION("2D Rectangular Array Tx Digital Beam Synthesis");
	//SET_MODEL_SYMBOL("SYM_RADAR_Tx_DBS_2D");
	SET_MODEL_CATEGORY("Array Signal Processing");

	// --------- 端口 ---------
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetDescription("input signal");
	}
	{
		auto p = ADD_MODEL_INPUT(InTheta);
		p.SetDescription("array direction angle theta in RADIANS");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(InPhi);
		p.SetDescription("array direction angle phi in RADIANS");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output signal");
	}

	// --------- 参数 ---------
	{
		auto p = ADD_MODEL_PARAM(NumOfAntx);
		p.SetDefaultValue("4");
		p.SetDescription("Number of Antenna in X axis");
	}
	{
		auto p = ADD_MODEL_PARAM(NumOfAnty);
		p.SetDefaultValue("4");
		p.SetDescription("Number of Antenna in Y axis");
	}
	{
		auto p = ADD_MODEL_PARAM(Dx);
		p.SetDefaultValue("0.5");
		p.SetDescription("Antenna Spacing in wavelengths of X axis");
	}
	{
		auto p = ADD_MODEL_PARAM(Dy);
		p.SetDefaultValue("0.5");
		p.SetDescription("Antenna Spacing in wavelengths of Y axis");
	}
	{
		auto p = ADD_MODEL_PARAM(Theta);
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("Array direction angle subtended from Z axis to point R (target point in space)");
	}
	{
		auto p = ADD_MODEL_PARAM(Phi);
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("Array direction angle between the projection of R (target pointin space) onto the X-Y plane and the X axis");
	}

	// Window_Type
	{
		auto p = ADD_MODEL_ENUM_PARAM(Window_Type, Window_TypeEnum);
		p.AddEnumeration("Rectangle", RADAR_Tx_DBS_2D::Rectangle);
		p.AddEnumeration("Bartlett", RADAR_Tx_DBS_2D::Bartlett);
		p.AddEnumeration("Hanning", RADAR_Tx_DBS_2D::Hanning);
		p.AddEnumeration("Hamming", RADAR_Tx_DBS_2D::Hamming);
		p.AddEnumeration("Blackman", RADAR_Tx_DBS_2D::Blackman);
		p.AddEnumeration("SteepBlackman", RADAR_Tx_DBS_2D::SteepBlackman);
		p.AddEnumeration("Kaiser", RADAR_Tx_DBS_2D::Kaiser);
		p.SetDefaultValue("Rectangle");
		p.SetDescription("windowing type");
	}

	// Kaiser
	{
		auto p = ADD_MODEL_PARAM(WindowParameters);
		p.SetDefaultValue("0");
		p.SetDescription(
			"the value Beta defined in Kaiser window function which is a non-negative value to determine the Kaiser window shape."
			"Beta=Alpah*Pl. This parameter is only used when Kaiser window is used in Window_Type");
		p.SetHideCondition("Window_Type ~= 6");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_Tx_DBS_2D::RADAR_Tx_DBS_2D()
	: NumOfAntx(4)
	, NumOfAnty(4)
	, Dx(0.5)
	, Dy(0.5)
	, Theta(0.0)
	, Phi(0.0)
	, Window_Type(Rectangle)
	, WindowParameters(0.0)
{
}

// Kaiser I0
double RADAR_Tx_DBS_2D::i0_bessel(double x)
{
	static const double i0A[] = {
		-4.41534164647933937950E-18,
		 3.33079451882223809783E-17,
		-2.43127984654795469359E-16,
		 1.71539128555513303061E-15,
		-1.16853328779934516808E-14,
		 7.67618549860493561688E-14,
		-4.85644678311192946090E-13,
		 2.95505266312963983461E-12,
		-1.72682629144155570723E-11,
		 9.67580903537323691224E-11,
		-5.18979560163526290666E-10,
		 2.65982372468238665035E-9,
		-1.30002500998624804212E-8,
		 6.04699502254191894932E-8,
		-2.67079385394061173391E-7,
		 1.11738753912010371815E-6,
		-4.41673835845875056359E-6,
		 1.64484480707288970893E-5,
		-5.75419501008210370398E-5,
		 1.88502885095841655729E-4,
		-5.76375574538582365885E-4,
		 1.63947561694133579842E-3,
		-4.32430999505057594430E-3,
		 1.05464603945949983183E-2,
		-2.37374148058994688156E-2,
		 4.93052842396707084878E-2,
		-9.49010970480476444210E-2,
		 1.71620901522208775349E-1,
		-3.04682672343198398683E-1,
		 6.76795274409476084995E-1
	};

	static const double i0B[] = {
		-7.23318048787475395456E-18,
		-4.83050448594418207126E-18,
		 4.46562142029675999901E-17,
		 3.46122286769746109310E-17,
		-2.82762398051658348494E-16,
		-3.42548561967721913462E-16,
		 1.77256013305652638360E-15,
		 3.81168066935262242075E-15,
		-9.55484669882830764870E-15,
		-4.15056934728722208663E-14,
		 1.54008621752140982691E-14,
		 3.85277838274214270114E-13,
		 7.18012445138366623367E-13,
		-1.79417853150680611778E-12,
		-1.32158118404477131188E-11,
		-3.14991652796324136454E-11,
		 1.18891471078464383424E-11,
		 4.94060238822496958910E-10,
		 3.39623202570838634515E-9,
		 2.26666899049817806459E-8,
		 2.04891858946906374183E-7,
		 2.89137052083475648297E-6,
		 6.88975834691682398426E-5,
		 3.36911647825569408990E-3,
		 8.04490411014108831608E-1
	};

	auto chbevl = [](double xx, const double* coef, int n) -> double {
		double b0 = coef[0];
		double b1 = 0.0;
		double b2 = 0.0;
		for (int i = 1; i < n; ++i)
		{
			b2 = b1;
			b1 = b0;
			b0 = xx * b1 - b2 + coef[i];
		}
		return 0.5 * (b0 - b2);
	};

	const double ax = std::fabs(x);
	if (ax <= 8.0)
	{
		// exp(x) * chbevl(x/2 - 2, A)
		const double y = chbevl(ax / 2.0 - 2.0, i0A, (int)(sizeof(i0A) / sizeof(i0A[0])));
		return std::exp(ax) * y;
	}
	else
	{
		// exp(x) * chbevl(32/x - 2, B) / sqrt(x)
		const double y = chbevl(32.0 / ax - 2.0, i0B, (int)(sizeof(i0B) / sizeof(i0B[0])));
		return std::exp(ax) * y / std::sqrt(ax);
	}
}

void RADAR_Tx_DBS_2D::make_window(Window_TypeEnum type, int L, double beta, std::vector<double>& w)
{
	w.assign(std::max(L, 1), 1.0);
	if (L <= 1) { w[0] = 1.0; return; }

	auto omega = [L](int p) -> double {
		// 对称采样 denom = (L-1)
		return kTwoPi * static_cast<double>(p) / static_cast<double>(L - 1);
	};

	switch (type)
	{
	case Rectangle:
		for (int p = 0; p < L; ++p) w[p] = 1.0;
		break;

	case Bartlett:
		for (int p = 0; p < L; ++p)
		{
			const double mid = 0.5 * (L - 1);
			double val = 1.0 - std::fabs((p - mid) / mid);
			if (val < 0.0) val = 0.0;
			w[p] = val;
		}
		break;

	case Hanning:
		for (int p = 0; p < L; ++p)
		{
			const double th = omega(p);
			w[p] = 0.5 * (1.0 - std::cos(th));
		}
		break;

	case Hamming:
		for (int p = 0; p < L; ++p)
		{
			const double th = omega(p);
			w[p] = 0.54 - 0.46 * std::cos(th);
		}
		break;

	case Blackman:
		for (int p = 0; p < L; ++p)
		{
			const double th = omega(p);
			w[p] = 0.42 - 0.5 * std::cos(th) + 0.08 * std::cos(2.0 * th);
		}
		break;

	case Kaiser:
	{
		const double b = std::max(beta, 0.0);
		const double denom = i0_bessel(b);
		for (int p = 0; p < L; ++p)
		{
			const double t = 2.0 * p / static_cast<double>(L - 1) - 1.0;
			w[p] = i0_bessel(b * std::sqrt(std::max(0.0, 1.0 - t * t))) / denom;
		}
	}
	break;

	case SteepBlackman:
	{
		// ===== SteepBlackman=====
		// 1) 采用 4-term cosine-sum（Blackman-Harris / Steep Blackman）
		//    w[n] = a0 - a1*cos(2π n/N) + a2*cos(4π n/N) - a3*cos(6π n/N)
		// 2) 对称折返：n = (p < L/2) ? p : (L - p - 1)
		// 3) N = L - 1（symmetric 采样分母）

		// 参考系数
		const double a0 = 0.35875;
		const double a1 = 0.48829;
		const double a2 = 0.14128;
		const double a3 = 0.01168;

		// L<=1 时，窗只有一个点。参考实现里一般可认为为 0 或 1。
		// 这里更保守：保持为 0
		if (L <= 1)
		{
			w.assign(std::max(L, 1), 0.0);
			break;
		}

		const double PI = std::acos(-1.0);
		const double N = static_cast<double>(L - 1);

		w.assign(L, 0.0);

		for (int p = 0; p < L; ++p)
		{
			// 对称折返索引
			const int n = (p < (L / 2)) ? p : (L - p - 1);

			// 角度项：2π*n/N
			const double th = 2.0 * PI * static_cast<double>(n) / N;

			// 4-term SteepBlackman
			double val = a0
				- a1 * std::cos(th)
				+ a2 * std::cos(2.0 * th)
				- a3 * std::cos(3.0 * th);

			// 裁负
			//if (val < 0.0) val = 0.0;

			w[p] = val;
		}
	}
	break;

	default:
		for (int p = 0; p < L; ++p) w[p] = 1.0;
		break;
	}
}

void RADAR_Tx_DBS_2D::rebuild_cache_()
{
	nx_ = std::max(1, NumOfAntx);
	ny_ = std::max(1, NumOfAnty);
	nChExpected_ = nx_ * ny_;

	xPos_.assign(nx_, 0.0);
	yPos_.assign(ny_, 0.0);

	for (int m = 0; m < nx_; ++m) xPos_[m] = static_cast<double>(m) * Dx;
	for (int n = 0; n < ny_; ++n) yPos_[n] = static_cast<double>(n) * Dy;

	make_window(Window_Type, nx_, WindowParameters, wx_);
	make_window(Window_Type, ny_, WindowParameters, wy_);

	// row-major / x-fast：idx = n*Nx + m
	taper2d_.assign(nChExpected_, 1.0);
	for (int n = 0; n < ny_; ++n)
	{
		for (int m = 0; m < nx_; ++m)
		{
			const int idx = n * nx_ + m;
			taper2d_[idx] = wx_[m] * wy_[n];
		}
	}
}

bool RADAR_Tx_DBS_2D::Setup()
{
	thetaConnected_ = InTheta.IsConnected();
	phiConnected_ = InPhi.IsConnected();

	rebuild_cache_();

	const size_t busSize = output.GetSize();
	if (input.GetSampleRate() > 0.0)
	{
		for (size_t i = 0; i < busSize; ++i)
			output[i].SetSampleRate(input.GetSampleRate());
	}
	return true;
}

bool RADAR_Tx_DBS_2D::Run()
{
	// 驱动时间轴对齐
	(void)input.GetTime(0, GetCount());

	const Cx xin = input[0];

	// 角度获取
	double theta = thetaConnected_ ? InTheta[0] : Theta;
	double phi = phiConnected_ ? InPhi[0] : Phi;

	// 是否转弧度判断
	if (!thetaConnected_ && std::fabs(theta) > kTwoPi) theta = deg2rad(theta);
	if (!phiConnected_   && std::fabs(phi) > kTwoPi) phi = deg2rad(phi);

	const double sTh = std::sin(theta);
	const double kx = sTh * std::cos(phi);
	const double ky = sTh * std::sin(phi);

	const size_t busSize = output.GetSize();
	const size_t nWrite = std::min(busSize, static_cast<size_t>(nChExpected_));

	for (size_t idx = 0; idx < nWrite; ++idx)
	{
		// row-major / x-fast
		const int n = static_cast<int>(idx / nx_);
		const int m = static_cast<int>(idx % nx_);

		const double psi = kTwoPi * (xPos_[m] * kx + yPos_[n] * ky);

		// exp(-j*psi)
		const double cs = std::cos(psi);
		const double sn = std::sin(psi);
		const Cx phase(cs, -sn);

		output[idx][0] = xin * (taper2d_[idx] * phase);
	}

	for (size_t idx = nWrite; idx < busSize; ++idx)
		output[idx][0] = Cx(0.0, 0.0);

	return true;
}