#include "RADAR_CFAR_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_CFAR_M)
{
	SET_MODEL_DESCRIPTION("Constant False Alarm Rate");
	SET_MODEL_SYMBOL("SYM_RADAR_CFAR_M@RADAR Symbols");
	SET_MODEL_CATEGORY("RADAR Models");

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
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CFAR_Type, SelectedCFARType);
		enumParam.SetDescription("The type of CFAR: CA, SOCA, GOCA, Clutter Map");
		enumParam.AddEnumeration("CA", CA);
		enumParam.AddEnumeration("SOCA", SOCA);
		enumParam.AddEnumeration("GOCA", GOCA);
		enumParam.AddEnumeration("Clutter Map", ClutterMap);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CFAR_Dimension, SelectedCFARDimension);
		enumParam.SetDescription("The type of CFAR dimension: Range, Doppler");
		enumParam.AddEnumeration("Range", Range);
		enumParam.AddEnumeration("Doppler", Doppler);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(CellSize);
		param.SetDescription("Cell size of samples which will be detected");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ReferenceCell);
		param.SetDescription("The number of samples/range bins which are regarded as Reference Cell");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GuardCell);
		param.SetDescription("The number of samples/range bins which are regarded as Guard Cell");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Detector_Type, SelectedDetectorType);
		enumParam.SetDescription("Type of the detector: Envelop, Square, LogSquare, Log");
		enumParam.AddEnumeration("Envelop", Envelope);
		enumParam.AddEnumeration("Square", Square);
		enumParam.AddEnumeration("LogSquare", LogSquare);
		enumParam.AddEnumeration("Log", Log);
		enumParam.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Threshold, SelectedThresholdType);
		enumParam.SetDescription("The method to decide the threshold: Pf, Alpha");
		enumParam.AddEnumeration("Pf", ThresholdByPf);
		enumParam.AddEnumeration("Alpha", ThresholdByAlpha);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pf);
		param.SetDescription("Expected False Alarm Rate");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1e-4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Alpha);
		param.SetDescription("Threshold Accumulation Factor");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		// 矩阵版帮助文档中 Alpha 始终作为可见参数列出，这里不设置显隐规则。
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Beta);
		param.SetDescription("Adaption coeficient for clutter map");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
	}

	return true;
}
#endif

RADAR_CFAR_M::RADAR_CFAR_M()
	: CFAR_Type(CA)
	, CFAR_Dimension(Range)
	, CellSize(100)
	, ReferenceCell(32)
	, GuardCell(4)
	, Detector_Type(Square)
	, Threshold(ThresholdByPf)
	, Pf(1e-4)
	, Alpha(1.0)
	, Beta(1.0)
	, ThresholdFactor(1.0)
	, clutterMapInitialized_(false)
{
}

// 求解 SOCA-CFAR 虚警率表达式
// alpha：门限因子
// N：参考窗总点数
// 说明：保留普通版 RADAR_CFAR 的函数形式，便于后续黑盒拟合内置门限因子
// 当前默认 Setup 中仍采用 CA 形式近似 SOCA/GOCA，与给出的普通版代码保持一致。
double RADAR_CFAR_M::SOFactor(double alpha, int N)
{
	double pfa = 0.0;
	int n = N / 2;
	for (int i = 0; i < n; i++)
	{
		pfa += 2.0 * std::tgamma(n + i) / std::tgamma(i + 1) / std::tgamma(n)
			* std::pow((2.0 + alpha / n), -n - i);
	}
	return pfa;
}

// 求解 GOCA-CFAR 虚警率表达式
// alpha：门限因子
// N：参考窗总点数
double RADAR_CFAR_M::GOFactor(double alpha, int N)
{
	double pfa = 0.0;
	int n = N / 2;
	for (int i = 0; i < n; i++)
	{
		pfa -= 2.0 * std::tgamma(n + i) / std::tgamma(i + 1) / std::tgamma(n)
			* std::pow((2.0 + alpha / n), -n - i);
	}
	pfa += 2.0 * std::pow((1.0 + alpha / n), -n);
	return pfa;
}

// Clutter Map 点杂波图门限因子表达式
double RADAR_CFAR_M::ClutterMapPointFactor(double alpha, int m, double r)
{
	double pfa = 1.0;
	for (int n = 0; n < m; n++)
	{
		pfa *= std::pow(1.0 + alpha * r * std::pow((1.0 - r), n), -1.0);
	}
	return pfa;
}

// Clutter Map 面杂波图门限因子表达式
double RADAR_CFAR_M::ClutterMapPlaneFactor(double alpha, int m, double r, double M)
{
	double pfa = 1.0;
	for (int n = 0; n < m; n++)
	{
		pfa *= std::pow(1.0 + alpha * r * std::pow((1.0 - r), n) / M, -1.0);
	}
	return pfa;
}

// 二分法求解门限因子：保留普通版代码结构，便于需要时替换 SOCA/GOCA 的门限因子求法
double RADAR_CFAR_M::SolutionBinaray(double pf, int N, double a, double b, double precision, SelectedCFARType cfarType)
{
	while (true)
	{
		double mean_ab = (a + b) / 2.0;
		double pfa = 0.0;

		if (cfarType == GOCA)
		{
			pfa = GOFactor(mean_ab, N);
		}
		else if (cfarType == SOCA)
		{
			pfa = SOFactor(mean_ab, N);
		}
		else
		{
			pfa = std::pow(1.0 + mean_ab / N, -N);
		}

		if (pfa <= 0.0)
			return mean_ab;

		double difference = 1.0 / pf - 1.0 / pfa;

		if (std::abs(difference) < precision || std::abs(a - b) < pf)
		{
			return mean_ab;
		}
		else if (difference < 0.0)
		{
			a = mean_ab;
		}
		else
		{
			b = mean_ab;
		}
	}
}

int RADAR_CFAR_M::numRows(const SystemVueModelBuilder::Matrix<double>& m) const
{
	return static_cast<int>(m.NumRows());
}

int RADAR_CFAR_M::numCols(const SystemVueModelBuilder::Matrix<double>& m) const
{
	return static_cast<int>(m.NumColumns());
}

// 检波器
// 注意：给出的普通版 RADAR_CFAR 代码里 Square / LogSquare / Log 实际上没有再次平方或取 log，
// 而是把输入看成已经经过外部 detector 后的实数视频量。
// 这里为了更贴近 RADAR_CFAR_M 帮助文档，提供完整 detector law；
// 若你的验证链路前面已经接 RADAR_Detector_M，则建议 RADAR_CFAR_M 选择 Envelop，避免重复检波。
double RADAR_CFAR_M::detectorLaw(double x) const
{
	double ax = std::abs(x);
	const double eps = 1e-300;

	switch (Detector_Type)
	{
	case Envelope:
		return ax;

	case Square:
		return ax * ax;

	case LogSquare:
		return std::log(std::max(ax * ax, eps));

	case Log:
		return std::log(std::max(ax, eps));

	default:
		return ax;
	}
}

// 门限因子
// Threshold=Alpha：直接使用 Alpha，便于做一一对齐黑盒验证。
// Threshold=Pf：使用标准 CA-CFAR 近似因子；SOCA/GOCA 后续可用 SolutionBinaray 精调。
double RADAR_CFAR_M::computeThresholdFactor() const
{
	if (Threshold == ThresholdByAlpha)
		return Alpha;

	int N = 2 * ReferenceCell;
	if (N <= 0 || Pf <= 0.0 || Pf >= 1.0)
		return 1.0;

	// 与给出的普通版 RADAR_CFAR 保持同一门限因子写法：2*ReferenceCell*(Pf^(-1/(2*ReferenceCell))-1)
	return static_cast<double>(N) * (std::pow(Pf, -1.0 / static_cast<double>(N)) - 1.0);
}

bool RADAR_CFAR_M::Setup()
{
	bool bStatus = true;

	// 矩阵版端口速率固定为 1 Matrix
	input.SetRate(1);
	output.SetRate(1);
	threshold.SetRate(1);

	if (CFAR_Type != ClutterMap)
	{
		if (CellSize <= 0)
		{
			POST_ERROR("CellSize must be greater than 0.");
			bStatus = false;
		}

		if (ReferenceCell <= 0)
		{
			POST_ERROR("ReferenceCell must be greater than 0.");
			bStatus = false;
		}

		if (GuardCell < 0)
		{
			POST_ERROR("GuardCell must be greater than or equal to 0.");
			bStatus = false;
		}

		ThresholdFactor = computeThresholdFactor();
	}
	else
	{
		// Clutter Map 使用 Alpha 和 Beta，不使用 CellSize / ReferenceCell / GuardCell / Pf
		ThresholdFactor = Alpha;
	}

	return bStatus;
}

// 处理一条一维序列：CA / SOCA / GOCA
void RADAR_CFAR_M::processOneVector(
	const std::vector<double>& rawLine,
	std::vector<double>& outLine,
	std::vector<double>& thLine,
	std::vector<double>* /*clutterLine*/)
{
	const int len = static_cast<int>(rawLine.size());
	outLine.assign(len, 0.0);
	thLine.assign(len, 0.0);

	if (len <= 0)
		return;

	std::vector<double> detLine(len, 0.0);
	for (int i = 0; i < len; ++i)
		detLine[i] = detectorLaw(rawLine[i]);

	// 与给出的普通版 RADAR_CFAR 一致：使用拼接/循环边界法，而不是边缘补零法。
	for (int i = 0; i < len; ++i)
	{
		double leadingSum = 0.0;
		double laggingSum = 0.0;

		for (int n = 0; n < ReferenceCell; ++n)
		{
			int leadingIdx = i + n - GuardCell - ReferenceCell;
			int laggingIdx = i + n + GuardCell + 1;

			while (leadingIdx < 0)
				leadingIdx += len;
			while (leadingIdx >= len)
				leadingIdx -= len;

			while (laggingIdx < 0)
				laggingIdx += len;
			while (laggingIdx >= len)
				laggingIdx -= len;

			leadingSum += detLine[leadingIdx];
			laggingSum += detLine[laggingIdx];
		}

		double leadingAvg = leadingSum / static_cast<double>(ReferenceCell);
		double laggingAvg = laggingSum / static_cast<double>(ReferenceCell);
		double z = 0.0;

		switch (CFAR_Type)
		{
		case CA:
			z = (leadingAvg + laggingAvg) / 2.0;
			break;

		case SOCA:
			z = std::min(leadingAvg, laggingAvg);
			break;

		case GOCA:
			z = std::max(leadingAvg, laggingAvg);
			break;

		default:
			z = (leadingAvg + laggingAvg) / 2.0;
			break;
		}

		thLine[i] = ThresholdFactor * z;

		// 与普通版 RADAR_CFAR 保持一致：比较原始输入值与 threshold，超过门限则输出原始输入值，否则输出 0。
		// 若后续黑盒发现内置比较 detector 输出，可将 rawLine[i] 替换为 detLine[i]。
		if (rawLine[i] > thLine[i])
			outLine[i] = rawLine[i];
		else
			outLine[i] = 0.0;
	}
}

// Clutter Map：使用一阶递归杂波图
void RADAR_CFAR_M::processOneVectorClutterMap(
	const std::vector<double>& rawLine,
	std::vector<double>& outLine,
	std::vector<double>& thLine,
	std::vector<double>& clutterLine)
{
	const int len = static_cast<int>(rawLine.size());
	outLine.assign(len, 0.0);
	thLine.assign(len, 0.0);

	if (len <= 0)
		return;

	if (static_cast<int>(clutterLine.size()) != len)
		clutterLine.assign(len, 0.0);

	for (int i = 0; i < len; ++i)
	{
		double det = detectorLaw(rawLine[i]);

		// 帮助文档只说明 simple first-order recursive filter，没有给出 Beta 的方向。
		// 这里采用 Beta 越大，越快跟随当前帧的写法：Cnew = (1-Beta)*Cold + Beta*Current。
		double beta = Beta;
		if (beta < 0.0) beta = 0.0;
		if (beta > 1.0) beta = 1.0;

		double cNew = (1.0 - beta) * clutterLine[i] + beta * det;
		clutterLine[i] = cNew;

		thLine[i] = Alpha * cNew;

		if (det > thLine[i])
			outLine[i] = rawLine[i];
		else
			outLine[i] = 0.0;
	}
}

bool RADAR_CFAR_M::Run()
{
	const SystemVueModelBuilder::Matrix<double>& inMat = input[0];

	const int nRows = numRows(inMat);
	const int nCols = numCols(inMat);

	SystemVueModelBuilder::Matrix<double> outMat;
	SystemVueModelBuilder::Matrix<double> thMat;
	outMat.Resize(nRows, nCols);
	thMat.Resize(nRows, nCols);

	if (nRows <= 0 || nCols <= 0)
	{
		output[0] = outMat;
		threshold[0] = thMat;
		return true;
	}

	// Clutter Map 状态矩阵初始化 / 尺寸变化时重置
	if (CFAR_Type == ClutterMap)
	{
		if (!clutterMapInitialized_ || numRows(clutterMap_) != nRows || numCols(clutterMap_) != nCols)
		{
			clutterMap_.Resize(nRows, nCols);
			for (int r = 0; r < nRows; ++r)
			{
				for (int c = 0; c < nCols; ++c)
					clutterMap_(r, c) = 0.0;
			}
			clutterMapInitialized_ = true;
		}
	}
	else
	{
		clutterMapInitialized_ = false;
	}

	if (CFAR_Dimension == Range)
	{
		// Range 方向：默认按每一列独立处理行索引，即每个 Doppler bin 沿 Range 做 CFAR。
		for (int c = 0; c < nCols; ++c)
		{
			std::vector<double> rawLine(nRows, 0.0);
			std::vector<double> outLine;
			std::vector<double> thLine;
			std::vector<double> clutterLine;

			for (int r = 0; r < nRows; ++r)
				rawLine[r] = inMat(r, c);

			if (CFAR_Type == ClutterMap)
			{
				clutterLine.resize(nRows);
				for (int r = 0; r < nRows; ++r)
					clutterLine[r] = clutterMap_(r, c);

				processOneVectorClutterMap(rawLine, outLine, thLine, clutterLine);

				for (int r = 0; r < nRows; ++r)
					clutterMap_(r, c) = clutterLine[r];
			}
			else
			{
				processOneVector(rawLine, outLine, thLine, 0);
			}

			for (int r = 0; r < nRows; ++r)
			{
				outMat(r, c) = outLine[r];
				thMat(r, c) = thLine[r];
			}
		}
	}
	else
	{
		// Doppler 方向：默认按每一行独立处理列索引，即每个 Range bin 沿 Doppler 做 CFAR。
		for (int r = 0; r < nRows; ++r)
		{
			std::vector<double> rawLine(nCols, 0.0);
			std::vector<double> outLine;
			std::vector<double> thLine;
			std::vector<double> clutterLine;

			for (int c = 0; c < nCols; ++c)
				rawLine[c] = inMat(r, c);

			if (CFAR_Type == ClutterMap)
			{
				clutterLine.resize(nCols);
				for (int c = 0; c < nCols; ++c)
					clutterLine[c] = clutterMap_(r, c);

				processOneVectorClutterMap(rawLine, outLine, thLine, clutterLine);

				for (int c = 0; c < nCols; ++c)
					clutterMap_(r, c) = clutterLine[c];
			}
			else
			{
				processOneVector(rawLine, outLine, thLine, 0);
			}

			for (int c = 0; c < nCols; ++c)
			{
				outMat(r, c) = outLine[c];
				thMat(r, c) = thLine[c];
			}
		}
	}

	output[0] = outMat;
	threshold[0] = thMat;

	return true;
}
