#include "RADAR_RCS.h"
#include "BetaDistribution.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_RCS )
{	
	SET_MODEL_DESCRIPTION("Radar target RCS");

	SET_MODEL_CATEGORY("Environments");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Es);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(RCS);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Type, SelectedType);
		enumParam.AddEnumeration("Const Value", ConstValue);		//0
		enumParam.AddEnumeration("Uniform PDF", UniformPDF);		//1
		enumParam.AddEnumeration("Gaussian PDF", GaussianPDF);		//2
		enumParam.AddEnumeration("Rayleigh PDF", RayleighPDF);		//3
		enumParam.AddEnumeration("LogNormal PDF", LogNormalPDF);	//4
		enumParam.AddEnumeration("Exponential PDF", ExponentialPDF);//5
		enumParam.AddEnumeration("Weibull PDF", WeibullPDF);		//6
		enumParam.AddEnumeration("ChiSquared PDF", ChiSquaredPDF);	//7
		enumParam.AddEnumeration("Gamma PDF", GammaPDF);			//8
		enumParam.AddEnumeration("Beta PDF", BetaPDF);				//9
		enumParam.AddEnumeration("F PDF", FPDF);					//10
		enumParam.AddEnumeration("Binomial CDF", BinomialCDF);		//11
		enumParam.AddEnumeration("Poisson CDF", PoissonCDF);		//12
		enumParam.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(VA);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(VB);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("Type==0||Type==3||Type==5||Type==7||Type==12");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TStep);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0.0001");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(DurationTime);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

RADAR_RCS::RADAR_RCS()
{
	t = 0.0;
	GenFlag = true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_RCS::Run()
{
	if (TStep == 0)
	{
		TStep = Es.GetTimeStep();
	}

	// 每个时间区间 DurationTime 生成一个 RCS 值，同一个时间区间内 RCS 值不变

	if (t > DurationTime)
	{
		t -= DurationTime;
		GenFlag = true;
	}

	if (GenFlag)
	{
		std::random_device rd;	// 随机器
		std::mt19937 gen(rd()); // 梅森旋转生成种子
		// 各分布初始化

		switch (Type)
		{
			case ConstValue:
			{
				RCSinDuration = VA;
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case UniformPDF:
			{
				std::uniform_real_distribution<double>	dU(VA, VB);
				RCSinDuration = dU(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case GaussianPDF:
			{
				std::normal_distribution<double>	dN(VA, VB);
				RCSinDuration = dN(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case RayleighPDF:
			{
				int n = DurationTime / TStep;
				std::uniform_real_distribution<double>	dU(0, 1);
				RCSinDuration = VA * sqrt(-2.0 * log(dU(gen)));
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case LogNormalPDF:
			{
				std::lognormal_distribution<double>	dLN(VA, VB);
				RCSinDuration = dLN(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case ExponentialPDF:
			{
				std::exponential_distribution<double>	dE(VA);
				RCSinDuration = dE(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case WeibullPDF:
			{
				std::weibull_distribution<double>	dW(VA, VB);
				RCSinDuration = dW(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case ChiSquaredPDF:
			{
				std::chi_squared_distribution<double>	dX(VA);
				RCSinDuration = dX(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case GammaPDF:
			{
				std::gamma_distribution<double>	dG(VA, VB);
				RCSinDuration = dG(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case BetaPDF:
			{
                // 方法1：使用自定义的 BetaDistribution 类
                BetaDistribution<double> dB(VA, VB);
				RCSinDuration = dB(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case FPDF:
			{
				std::fisher_f_distribution<double>	dF(VA, VB);
				RCSinDuration = dF(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case BinomialCDF:
			{
				std::binomial_distribution<int>	dBi(int(VA), VB);
				RCSinDuration = dBi(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			case PoissonCDF:
			{
				std::poisson_distribution<int>	dP(VA);
				RCSinDuration = dP(gen);
				EsinDuration = sqrt(fabs(RCSinDuration));
				break;
			}
			default:
			{
				RCSinDuration = 0.0;
				EsinDuration = 0.0;
				break;
			}
		}
		GenFlag = false;
	}

	RCS[0] = RCSinDuration;
	Es[0] = EsinDuration;
	
	t += TStep;
	return true;
}
