#include "RADAR_CFAR.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_CFAR )
{	
	SET_MODEL_DESCRIPTION("Constant False Alarm Rate");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The CFAR result of input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(threshold);
		port.SetDescription("The threshold result of input signal");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CFARType, SelectedCFARType);
		enumParam.SetDescription("The type of CFAR: CA, SOCA, GOCA, OS, Clutter Map");
		enumParam.AddEnumeration("CA", CA);						// 0
		enumParam.AddEnumeration("SOCA", SOCA);					// 1
		enumParam.AddEnumeration("GOCA", GOCA);					// 2
		enumParam.AddEnumeration("OS", OS);						// 3
		enumParam.AddEnumeration("Clutter Map", ClutterMap);	// 4
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(CellSize);
		param.SetDescription("Cell size of samples which will be detected");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000");
		param.SetHideCondition("CFARType == 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ReferenceCell);
		param.SetDescription("The number of samples/range bins which are regarded as Reference Cell");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
		param.SetHideCondition("CFARType == 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GuardCell);
		param.SetDescription("The number of samples/range bins which are regarded as Guard Cell");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
		param.SetHideCondition("CFARType == 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(kOrder);
		param.SetDescription("The kth element of the ordered list is called the kth order statistic.");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("24");
		param.SetHideCondition("CFARType ~= 3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ThresholdScaleFactor);
		param.SetDescription("Threshold scale factor");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("14");
		param.SetHideCondition("CFARType ~= 3");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(DetectorType, SelectedDetectorType);
		enumParam.SetDescription("Type of the detector: Envelope, Square, LogSquare, Log");
		enumParam.AddEnumeration("Envelope", Envelope);		// 0
		enumParam.AddEnumeration("Square", Square);			// 1
		enumParam.AddEnumeration("LogSquare", LogSquare);	// 2
		enumParam.AddEnumeration("Log", Log);				// 3
		enumParam.SetDefaultValue("1");
		enumParam.SetHideCondition("CFARType == 3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pf);
		param.SetDescription("Expected False Alarm Rate");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1e-4");
		param.SetHideCondition("CFARType == 3 || CFARType == 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Alpha);
		param.SetDescription("Threshold Accumulation Factor");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("DetectorType ~= 2 && CFARType ~= 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Beta);
		param.SetDescription("Adaption coeficient for clutter map");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("CFARType ~= 4");
	}
	return true;
}
#endif

RADAR_CFAR::RADAR_CFAR()
{

}


// 求解SOCA-CFAR
// alpha：门限因子
// N：参考窗总点数
double RADAR_CFAR::SOFactor(double alpha, int N)
{
	double Pfa = 0;
	int n = N / 2;
	for (int i = 0; i < n; i++)
	{
		Pfa += 2 * std::tgamma(n + i) / std::tgamma(i + 1) / std::tgamma(n)*std::pow((2 + alpha / n), -n - i);
	}

	return Pfa;
}

// 求解GOCA-CFAR
// alpha：门限因子
// N：参考窗总点数
double RADAR_CFAR::GOFactor(double alpha, int N)
{
	double Pfa = 0;
	int n = N / 2;
	for (int i = 0; i < n; i++)
	{
		Pfa -= 2 * std::tgamma(n + i) / std::tgamma(i + 1) / std::tgamma(n)*std::pow((2 + alpha / n), -n - i);
	}
	Pfa += 2 * std::pow((1 + alpha / n), -n);

	return Pfa;
}

// SystemVue直接通过参数设置了门限因子，无需求解
// 求解OS-CFAR
// alpha：门限因子
// N：参考窗总点数
// k：门限参考点序位
//double RADAR_CFAR::OSFactor(double alpha, int N, double k)
//{
//	double Pfa = std::tgamma(N + 1)*std::tgamma(N - k + alpha + 1) / std::tgamma(N - k + 1) / std::tgamma(N + alpha + 1);
//	return Pfa;
//}

// 求解杂波图-点参数
// alpha：门限因子
// m：天线旋转周期
double RADAR_CFAR::ClutterMapPointFactor(double alpha, int m, double r)
{
	double Pfa = 1;
	for (int n = 0; n < m; n++)
	{
		Pfa *= std::pow(1 + alpha * r*std::pow((1 - r), n), -1);
	}
	return Pfa;
}

// 求解杂波图-面参数
// alpha：门限因子
// m：天线旋转周期
// M：参考单元数
double RADAR_CFAR::ClutterMapPlaneFactor(double alpha, int m, double r, double M)
{
	double Pfa = 1;
	for (int n = 0; n < m; n++)
	{
		Pfa *= std::pow(1 + alpha * r*std::pow((1 - r), n) / M, -1);
	}
	return Pfa;
}

// 二分法求解门限因子
// Pf：虚警率
// N：训练窗的总长度
// a：小值
// b：大值
// precision：求解精度，此处采用蒙特卡洛模拟法，即每1/Pf次模拟出现虚警的次数差
double RADAR_CFAR::SolutionBinaray(double Pf, int N, double a, double b, double precision, SelectedCFARType CFARType)
{
	while (true)
	{
		double mean_ab = (a + b) / 2;
		double Pfa = 0;
		double fa = 0;

		if (CFARType == GOCA)
		{
			Pfa = GOFactor(mean_ab, N);
			fa = GOFactor(a, N);
		}
		else if (CFARType == SOCA)
		{
			Pfa = SOFactor(mean_ab, N);
			fa = SOFactor(a, N);
		}
		//else if (CFARType == OS) 
		//{
		//	Pfa = OSFactor(mean_ab, N, k);
		//	fa = OSFactor(a, N, k);
		//}

		double difference = 1 / Pf - 1 / Pfa;

		// 求解精度达到时返回
		if (abs(difference) < precision || abs(a - b) < Pf)
		{
			return mean_ab;
		}
		else if (difference < 0)
		{
			a = mean_ab;
		}
		else
		{
			b = mean_ab;
		}
	}
}

bool RADAR_CFAR::Setup()
{
	bool bStatus = true;


	if (CellSize > 0)
	{
		input.SetRate(CellSize);
		output.SetRate(CellSize);
		threshold.SetRate(CellSize);
	}
	else
	{
		POST_ERROR("Port rate (CellSize) must be greater than 0.");
		bStatus = false;
	}

	if (kOrder <= 0 || kOrder > 2 * ReferenceCell)
	{
		POST_ERROR("kOrder must be greater than 0 and smaller than 2 * ReferenceCell");
		bStatus = false;
	}

	// 求解门限因子
	switch (CFARType)
	{
	case RADAR_CFAR::CA:
		ThresholdFactor = 2 * ReferenceCell*(pow(Pf, -1.0 / (2 * ReferenceCell)) - 1);
		break;
	case RADAR_CFAR::SOCA:
		ThresholdFactor = 2 * ReferenceCell*(pow(Pf, -1.0 / (2 * ReferenceCell)) - 1);///TODO
		//ThresholdFactor = SolutionBinaray(Pf, 2 * ReferenceCell, 0, 30, 1, SOCA);
		break;
	case RADAR_CFAR::GOCA:
		ThresholdFactor = 2 * ReferenceCell*(pow(Pf, -1.0 / (2 * ReferenceCell)) - 1);///TODO
		//ThresholdFactor = SolutionBinaray(Pf, 2 * ReferenceCell, 0, 30, 1, GOCA);
		break;
	case RADAR_CFAR::OS:
		ThresholdFactor = ThresholdScaleFactor;
		break;
	case RADAR_CFAR::ClutterMap:
		break;
	default:
		break;
	}

	return bStatus;
}



//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_CFAR::Run()
{
	SystemVueModelBuilder::Matrix<double>	DetectorOut(1, CellSize);
	// 此处是为了均衡门限以对应检波器的类型
	for (int i = 0; i < CellSize; i++)
	{
		switch (DetectorType)
		{
		case RADAR_CFAR::Envelope:
		{
			DetectorOut(i) = abs(input[i]);
			break;
		}
		case RADAR_CFAR::Square:
		{
			//DetectorOut(i) = pow(abs(input[i]), 2);
			DetectorOut(i) = abs(input[i]);
			break;
		}
		case RADAR_CFAR::LogSquare:
		{
			//DetectorOut(i) = Alpha * log(pow(abs(input[i]), 2));
			DetectorOut(i) = input[i];
			break;
		}
		case RADAR_CFAR::Log:
		{
			//DetectorOut(i) = log(abs(input[i]));
			DetectorOut(i) = input[i];
			break;
		}
		default:
			break;
		}
	}

	//-----------------------------------------------------------------------------------
	// 补零
	//int PaddedLen = CellSize + 2 * (ReferenceCell + GuardCell);
	//SystemVueModelBuilder::Matrix<double> CellPadded(1, CellSize + 2 * (ReferenceCell + GuardCell));

	//for (int i = 0; i < PaddedLen; i++)
	//{
	//	if (i >= ReferenceCell + GuardCell && i < CellSize + ReferenceCell + GuardCell)
	//	{
	//		CellPadded(i) = DetectorOut(i - ReferenceCell - GuardCell);
	//	}
	//	else
	//		CellPadded(i) = 0;
	//}

	SystemVueModelBuilder::Matrix<double> LeadingWindow(1, ReferenceCell);
	SystemVueModelBuilder::Matrix<double> LaggingWindow(1, ReferenceCell);
	for (int i = 0; i < CellSize; i++)
	{
		// 获取当前参考窗（补零法）
		//for (int n = 0; n < ReferenceCell; n++)
		//{
		//	LeadingWindow(n) = CellPadded(i + n);
		//	LaggingWindow(n) = CellPadded(i + ReferenceCell + 2 * GuardCell + 1 + n);
		//}

		//double LeadingAvg = 0;
		//double LaggingAvg = 0;
		//for (int n = 0; n < ReferenceCell; n++)
		//{
		//	LeadingAvg += LeadingWindow(n);
		//	LaggingAvg += LaggingWindow(n);
		//}
		//LeadingAvg /= ReferenceCell;
		//LaggingAvg /= ReferenceCell;


		// 获取当前参考窗（拼接法）
		for (int n = 0; n < ReferenceCell; n++)
		{
			int LeadingWindowIndex = i + n - GuardCell - ReferenceCell;
			int LaggingWindowIndex = i + n + GuardCell + 1;
			LeadingWindow(n) = DetectorOut(LeadingWindowIndex < 0 ? LeadingWindowIndex + CellSize : LeadingWindowIndex);
			LaggingWindow(n) = DetectorOut(LaggingWindowIndex >= CellSize ? LaggingWindowIndex - CellSize : LaggingWindowIndex);
		}

		double LeadingAvg = 0;
		double LaggingAvg = 0;
		for (int n = 0; n < ReferenceCell; n++)
		{
			LeadingAvg += LeadingWindow(n);
			LaggingAvg += LaggingWindow(n);
		}
		LeadingAvg /= ReferenceCell;
		LaggingAvg /= ReferenceCell;


		switch (CFARType)
		{
		case RADAR_CFAR::CA:
		{
			threshold[i] = ThresholdFactor * (LeadingAvg + LaggingAvg) / 2;
			break;
		}
		case RADAR_CFAR::SOCA:
		{
			threshold[i] = ThresholdFactor * std::min(LeadingAvg, LaggingAvg);
			break;
		}
		case RADAR_CFAR::GOCA:
		{
			threshold[i] = ThresholdFactor * std::max(LeadingAvg, LaggingAvg);
			break;
		}
		case RADAR_CFAR::OS:
		{
			std::vector<double>	ReferenceWindowOrder;
			for (int i = 0; i < ReferenceCell; i++)
			{
				ReferenceWindowOrder.push_back(LeadingWindow(i));
				ReferenceWindowOrder.push_back(LaggingWindow(i));
			}
			std::sort(ReferenceWindowOrder.begin(), ReferenceWindowOrder.end());

			threshold[i] = ThresholdFactor * ReferenceWindowOrder[kOrder - 1];
			break;
		}
		case RADAR_CFAR::ClutterMap:
		{
			break;
		}
		default:
			break;
		}

		// 门限比较输出
		if (input[i] > threshold[i])
		{
			output[i] = input[i];
		}
		else
		{
			output[i] = 0;
		}
	}
	

	return true;
}
