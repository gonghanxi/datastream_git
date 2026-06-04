#include "RADAR_Clutter_H.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_Clutter_H )
{	
	SET_MODEL_DESCRIPTION("Radar clutter simulation");

	SET_MODEL_CATEGORY("Environments");
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(ClutterSample);
		port.SetDescription("The clutter samples to evaluate the clutter performance");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The clutter");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Coeff);
		port.SetDescription("Filter Coeff");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RF_Freq);
		param.SetDescription("RF carrier frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1e9");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(PDF, SelectedPDF);
		enumParam.SetDescription("Clutter amplitude probability density: Rayleigh PDF, LogNormal PDF, Weibull PDF");
		enumParam.AddEnumeration("Rayleigh PDF", Rayleigh);
		enumParam.AddEnumeration("Lognormal PDF", Lognoraml);
		enumParam.AddEnumeration("Weibull PDF", Weibull);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(VA);
		param.SetDescription("Voltage value dependent on PDF");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(VB);
		param.SetDescription("Voltage value dependent on PDF");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("(PDF==0)");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(PSD, SelectedPSD);
		enumParam.SetDescription("Clutter power spectrum density: Gaussian PSD, Cauchy PSD, AllPole PSD");
		enumParam.AddEnumeration("Gaussian PSD", Gaussian);
		enumParam.AddEnumeration("Cauchy PSD", Cauchy);
		enumParam.AddEnumeration("Allpole PSD", Allpole);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PA);
		param.SetDescription("Parameter value dependent on PSD");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1.0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PB);
		param.SetDescription("Parameter value dependent on PSD");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("PSD == 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TStep);
		param.SetDescription("Simulation time step; TStep=0 results in use of externally set TStep");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-7");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FilterLen);
		param.SetDescription("The PSD filter length, also the fft size");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("24");
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(DurationTime); // DurationTime 即 PRI
		param.SetDescription("The PSD of clutter frequency range");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("100e-6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Vr);
		param.SetDescription("The relative radial velocity of interested target");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}
	return true;
}
#endif

RADAR_Clutter_H::RADAR_Clutter_H()
{

}

ERESULT RADAR_Clutter_H::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	if (RF_Freq > 0)
	{
		output.SetCharacterizationFrequency(RF_Freq);
		ClutterSample.SetCharacterizationFrequency(RF_Freq);
	}
	else
	{
		POST_ERROR("The characterizatuon frequency RF_Freq must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Port rate should be set here
//-----------------------------------------------------------------------------------
bool RADAR_Clutter_H::Setup()
{
	bool bStatus = true;

	if (FilterLen > 0)
	{
		Coeff.SetRate(FilterLen);
	}
	else
	{
		POST_ERROR("FilterLen must be greater than 0.");
		bStatus = false;
	}

	if (TStep <= 0)
	{
		POST_ERROR("TStep must be greater than 0.");
		bStatus = false;
	}

	if (TStep > DurationTime)
	{
		POST_ERROR("TStep must be <= DurationTime");
		bStatus = false;
	}

	num_sample = DurationTime / TStep;

	input.SetRate(num_sample);
	output.SetRate(num_sample);
	ClutterSample.SetRate(num_sample);

	return bStatus;
}

// 计算均值
double average(SystemVueModelBuilder::Matrix<double>& A, int LenA)
{
	double sum = 0;
	for (int i = 0; i < LenA; i++)
	{
		sum += A(i);
	}
	return sum / LenA;
}

// 计算方差
double variance(SystemVueModelBuilder::Matrix<double>& A, int LenA)
{
	double avg = average(A, LenA);
	double sum = 0.0;
	for (int i = 0; i < LenA; i++)
	{
		sum += std::pow(A(i) - avg, 2);
	}
	return sum / LenA;
}

// 计算标准差
double standardDeviation(SystemVueModelBuilder::Matrix<double>& A, int LenA)
{
	return std::sqrt(variance(A, LenA));
}

// 常规卷积
SystemVueModelBuilder::Matrix<double> convolve(SystemVueModelBuilder::Matrix<double>& A, SystemVueModelBuilder::Matrix<double>& B, int LenA, int LenB)
{
	SystemVueModelBuilder::Matrix<double> result(1, LenA + LenB - 1);
	result.Zero();

	for (int i = 0; i < LenA; ++i)
	{
		for (int j = 0; j < LenB; ++j)
		{
			result(i + j) += A(i)*B(j);
		}
	}
	return result;
}


//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------

bool RADAR_Clutter_H::Run()
{
	const double PI = std::acos(-1);
	const std::complex<double> imag_I(0, 1);

	if (TStep == 0)
	{
		TStep = output.GetTimeStep();
	}

	// 脉冲重复频率
	double fr = 1 / DurationTime;

	std::random_device rd; // random seed
	std::mt19937 gen(rd()); //mersenne twister
	std::uniform_real_distribution<>	d1(0, 1);
	std::uniform_real_distribution<>	d2(0, 1);

	SystemVueModelBuilder::Matrix<double>	xi(1, num_sample);
	SystemVueModelBuilder::Matrix<double>	xq(1, num_sample);
	xi.Zero();
	xq.Zero();

	for (int i = 0; i < num_sample; i++)
	{
		xi(i) = 2.0 * (std::sqrt(-2.0 * std::log(d1(gen)))) * std::cos(2.0 * PI * d2(gen));
		xq(i) = 2.0 * (std::sqrt(-2.0 * std::log(d1(gen)))) * std::sin(2.0 * PI * d2(gen));
	}
	
	// clutter PSD

	// const double c = 299792458;
	const double c = 3e8;

	int	coe_num = FilterLen / 2;
	SystemVueModelBuilder::Matrix<double> b(1, FilterLen);
	SystemVueModelBuilder::Matrix<double> b_half(1, coe_num);
	b.Zero();
	b_half.Zero();

	switch (PSD)
	{
		case Gaussian:
		{
			// sf = exp(-f * *2 / (2 * PA * *2));

			double	lambda0 = c / RF_Freq;	//clutter wavelength
			double	sigmav = PA;
			double	sigmaf = 2 * sigmav / lambda0;

			// filter response
			for (int n = 0; n < coe_num; n++)
			{
				b_half(n) = 2.0 * sigmaf*std::sqrt(PI)*std::exp(-4.0*sigmaf*sigmaf*PI*PI*n*n / (fr*fr)) / fr;
			}

			for (int n = 0; n < FilterLen; n++)
			{
				if (n < coe_num)
				{
					b(n) = 0.5 * b_half(coe_num - n - 1);
				}
				else
				{
					b(n) = 0.5 * b_half(n - coe_num);
				}
				Coeff[n] = b(n);
			}
			break;
		}
		case Cauchy:
		{
			break;
		}
		case Allpole:
		{
			break;
		}
	}

	// clutter PDF
	SystemVueModelBuilder::Matrix<double>	yyi(1, num_sample);
	SystemVueModelBuilder::Matrix<double>	yyq(1, num_sample);
	SystemVueModelBuilder::Matrix< std::complex<double> >	yy(1, num_sample);
	SystemVueModelBuilder::Matrix< double >	xiconv(1, num_sample + FilterLen - 1);
	SystemVueModelBuilder::Matrix< double >	xqconv(1, num_sample + FilterLen - 1);
	SystemVueModelBuilder::Matrix< double >	xxi(1, num_sample);
	SystemVueModelBuilder::Matrix< double >	xxq(1, num_sample);
	yyi.Zero();
	yyq.Zero();
	yy.Zero();
	xiconv.Zero();
	xqconv.Zero();
	xxi.Zero();
	xxq.Zero();

	xiconv = convolve(b, xi, FilterLen, num_sample);
	xqconv = convolve(b, xq, FilterLen, num_sample);

	// 去除暂态响应
	for (int i = 0; i < num_sample; i++)
	{
		xxi(i) = xiconv(i + FilterLen - 1);
		xxq(i) = xqconv(i + FilterLen - 1);
	}

	// 求解统计量
	double xisigmac = standardDeviation(xxi, num_sample);
	double ximuc = average(xxi, num_sample);
	double xqsigmac = standardDeviation(xxq, num_sample);
	double xqmuc = average(xxq, num_sample);

	// 归一化
	for (int i = 0; i < num_sample; i++)
	{
		yyi(i) = (xxi(i) - ximuc) / xisigmac;
		yyq(i) = (xxq(i) - xqmuc) / xqsigmac;
	}

	switch (PDF)
	{
	case RADAR_Clutter_H::Rayleigh:
	{
		// fx = 2*x/VA * exp(-x**2/VA)

		double sigmac = VA;

		for (int i = 0; i < num_sample; i++)
		{
			yyi(i) = sigmac * yyi(i);
			yyq(i) = sigmac * yyq(i);
			yy(i) = yyi(i) + imag_I * yyq(i);
		}
		break;
	}
	case RADAR_Clutter_H::Lognoraml:
	{
		// fx = 1/(VA*x*(2*pi)**0.5) * exp(-(log(x) - log(VB))**2 / (2*VA**2))

		double	sigmac = VA;
		double	muc = VB;

		for (int i = 0; i < num_sample; i++)
		{
			yyi(i) = sigmac * yyi(i) + log(muc);
			yyq(i) = sigmac * yyq(i) + log(muc);
			yy(i) = std::exp(yyi(i) + imag_I * yyq(i));
		}
		break;
	}
	case RADAR_Clutter_H::Weibull:
	{
		// fx = VA/VB*(x/VB)**(VA-1) * exp(-(x/VB)**VA)

		double	p = VA;
		double	q = VB;
		double	sigmac = sqrt(pow(q, p) / 2);

		for (int i = 0; i < num_sample; ++i)
		{
			yyi(i) = sigmac * yyi(i);
			yyq(i) = sigmac * yyq(i);
			yy(i) = std::pow((yyi(i)*yyi(i)), 1 / p) + imag_I * std::pow((yyq(i)*yyq(i)), 1 / p);
		}
		break;
	}
	default:
		break;
	}

	for (int i = 0; i < num_sample; i++)
	{
		ClutterSample[i] = yy(i);
		output[i] = input[i].complex() * yy(i);
	}

	return true;
}
