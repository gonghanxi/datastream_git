#include "RADAR_PlotsCentroid.h"

#include <algorithm>
#include <cmath>
#include <queue>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_PlotsCentroid)
{
	SET_MODEL_DESCRIPTION("Plots Centroid");
	SET_MODEL_CATEGORY("Signal Processing");

	// ============================================================
	// 端口定义
	// ============================================================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("The detected result of input signal with binary detector");
	}

	// ============================================================
	// 参数定义
	// ============================================================
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(Type, RADAR_PlotsCentroid::CentroidTypeEnum);
		p.SetName("Type");
		p.AddEnumeration("1D", RADAR_PlotsCentroid::Type_1D);
		p.AddEnumeration("2D", RADAR_PlotsCentroid::Type_2D);
		// 内置帮助文档默认值为 2D，对应枚举值 1。
		p.SetDefaultValue("1");
		p.SetDescription("Centroid Type: 1D, 2D");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SampleNum);
		p.SetName("SampleNum");
		p.SetDefaultValue("1024");
		p.SetDescription("SampleNum is the number of data sample. The data is the CFAR output.");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(RangeBinNum);
		p.SetName("RangeBinNum");
		p.SetDefaultValue("512");
		p.SetDescription("PD radar range bin number");
		// 参数界面：Type=1D 时隐藏，Type=2D 时显示。
		// Type_2D 的枚举值为 1，因此条件写作 Type ~= 1。
		p.SetHideCondition("Type ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(DopplerBinNum);
		p.SetName("DopplerBinNum");
		p.SetDefaultValue("128");
		p.SetDescription("PD radar Doppler bin number");
		// 参数界面：Type=1D 时隐藏，Type=2D 时显示。
		p.SetHideCondition("Type ~= 1");
	}

	return true;
}
#endif


RADAR_PlotsCentroid::RADAR_PlotsCentroid()
	: input()
	, output()
	, Type(Type_2D)
	, SampleNum(1024)
	, RangeBinNum(512)
	, DopplerBinNum(128)
	, effectiveSampleNum_(1024)
{
}


// ============================================================
// Setup
// ============================================================
bool RADAR_PlotsCentroid::Setup()
{
	if (!validateAndPrepare_())
		return false;

	input.SetRate(static_cast<unsigned>(effectiveSampleNum_));
	output.SetRate(static_cast<unsigned>(effectiveSampleNum_));

	return true;
}


// ============================================================
// 参数检查与内部样本数计算
// ============================================================
bool RADAR_PlotsCentroid::validateAndPrepare_()
{
	if (Type == Type_1D)
	{
		if (SampleNum <= 0)
		{
			POST_ERROR("SampleNum must be greater than 0.");
			return false;
		}

		effectiveSampleNum_ = SampleNum;
		return true;
	}

	// Type=2D
	if (RangeBinNum <= 0)
	{
		POST_ERROR("RangeBinNum must be greater than 0 when Type is 2D.");
		return false;
	}

	if (DopplerBinNum <= 0)
	{
		POST_ERROR("DopplerBinNum must be greater than 0 when Type is 2D.");
		return false;
	}

	const long long total =
		static_cast<long long>(RangeBinNum) * static_cast<long long>(DopplerBinNum);

	if (total <= 0 || total > 2147483647LL)
	{
		POST_ERROR("RangeBinNum * DopplerBinNum is invalid or too large.");
		return false;
	}

	effectiveSampleNum_ = static_cast<int>(total);

	// 帮助文档说明 Type=2D 时 SampleNum 等于 RangeBinNum * DopplerBinNum。
	// 参数界面默认值可能不一致，因此这里不强制失败，只给出警告并以内部分辨率为准。
	if (SampleNum != effectiveSampleNum_)
	{
		POST_WARNING("For 2D mode, SampleNum should be equal to RangeBinNum * DopplerBinNum. "
			"The model will use RangeBinNum * DopplerBinNum as the effective port rate.");
	}

	return true;
}


// ============================================================
// Run
// ============================================================
bool RADAR_PlotsCentroid::Run()
{
	if (Type == Type_1D)
		run1D_();
	else
		run2D_();

	return true;
}


// ============================================================
// 1D 点迹质心
//
// 规则：
//   1. 连续 input[i] > 0 的区间视为一个 plot；
//   2. 用 input[i] 作为权重计算质心；
//   3. 质心索引四舍五入；
//   4. 在质心索引处写入该 plot 的最大值；
//   5. 其他位置清零。
// ============================================================
void RADAR_PlotsCentroid::run1D_()
{
	const int L = effectiveSampleNum_;

	for (int i = 0; i < L; ++i)
		output[i] = 0.0;

	int i = 0;
	while (i < L)
	{
		while (i < L && !isPositive_(input[i]))
			++i;

		if (i >= L)
			break;

		const int start = i;

		double sumW = 0.0;
		double sumIW = 0.0;
		double maxVal = input[i];

		while (i < L && isPositive_(input[i]))
		{
			const double w = input[i];

			sumW += w;
			sumIW += static_cast<double>(i) * w;
			if (w > maxVal)
				maxVal = w;

			++i;
		}

		const int end = i - 1;

		if (sumW > 0.0)
		{
			const double c = sumIW / sumW;
			int centroidIndex = roundToNearestIndex_(c);
			centroidIndex = clampInt_(centroidIndex, start, end);
			centroidIndex = clampInt_(centroidIndex, 0, L - 1);

			// 若多个 plot 极端情况下落到同一输出点，保留较大代表值。
			if (maxVal > output[centroidIndex])
				output[centroidIndex] = maxVal;
		}
	}
}


// ============================================================
// 2D 点迹质心
//
// 规则：
//   1. input > 0 的点视为候选点；
//   2. 使用 4 邻域连通域搜索形成二维 plot；
//   3. 对每个 plot 计算 Range / Doppler 的幅度加权质心；
//   4. 质心位置四舍五入到最近 bin；
//   5. 在质心位置写入该 plot 最大值，其余位置清零。
// ============================================================
void RADAR_PlotsCentroid::run2D_()
{
	const int R = RangeBinNum;
	const int D = DopplerBinNum;
	const int L = effectiveSampleNum_;

	for (int i = 0; i < L; ++i)
		output[i] = 0.0;

	std::vector<unsigned char> visited(static_cast<size_t>(L), 0u);

	// 4 邻域偏移：上下左右。
	// 黑盒测试结论：
	//   内置 RADAR_PlotsCentroid 不会把仅对角接触的两个正值点合并为一个 plot，
	//   因此这里不能使用 8 邻域。
	const int dr[4] = { 0, -1, 1,  0 };
	const int dd[4] = { -1,  0, 0,  1 };

	for (int d0 = 0; d0 < D; ++d0)
	{
		for (int r0 = 0; r0 < R; ++r0)
		{
			const int seed = idx2D_(r0, d0);

			if (visited[static_cast<size_t>(seed)] != 0u)
				continue;

			visited[static_cast<size_t>(seed)] = 1u;

			if (!isPositive_(input[seed]))
				continue;

			// 当前连通域的统计量
			double sumW = 0.0;
			double sumRW = 0.0;
			double sumDW = 0.0;
			double maxVal = input[seed];

			std::queue<int> q;
			q.push(seed);

			while (!q.empty())
			{
				const int idx = q.front();
				q.pop();

				const int d = idx / R;
				const int r = idx - d * R;

				const double w = input[idx];

				sumW += w;
				sumRW += static_cast<double>(r) * w;
				sumDW += static_cast<double>(d) * w;

				if (w > maxVal)
					maxVal = w;

				for (int k = 0; k < 4; ++k)
				{
					const int rn = r + dr[k];
					const int dn = d + dd[k];

					if (rn < 0 || rn >= R || dn < 0 || dn >= D)
						continue;

					const int ni = idx2D_(rn, dn);
					const size_t nsz = static_cast<size_t>(ni);

					if (visited[nsz] != 0u)
						continue;

					visited[nsz] = 1u;

					if (isPositive_(input[ni]))
						q.push(ni);
				}
			}

			if (sumW > 0.0)
			{
				int rc = roundToNearestIndex_(sumRW / sumW);
				int dc = roundToNearestIndex_(sumDW / sumW);

				rc = clampInt_(rc, 0, R - 1);
				dc = clampInt_(dc, 0, D - 1);

				const int outIdx = idx2D_(rc, dc);

				// 若多个 plot 极端情况下落到同一输出点，保留较大代表值。
				if (maxVal > output[outIdx])
					output[outIdx] = maxVal;
			}
		}
	}
}


// ============================================================
// 2D 索引展开
// ============================================================
int RADAR_PlotsCentroid::idx2D_(int rangeIndex, int dopplerIndex) const
{
	return dopplerIndex * RangeBinNum + rangeIndex;
}


// ============================================================
// plot 判据
// 由于该模块接在 CFAR 后、BinaryDetector 前，
// 帮助文档没有给 Threshold 参数，因此以正值作为候选点。
// ============================================================
bool RADAR_PlotsCentroid::isPositive_(double x) const
{
	return x > 0.0;
}


// ============================================================
// 四舍五入到最近整数索引
// 使用 floor(x+0.5)，避免不同编译器 round 行为差异。
// ============================================================
int RADAR_PlotsCentroid::roundToNearestIndex_(double x)
{
	if (!(x == x))
		return 0;

	return static_cast<int>(std::floor(x + 0.5));
}


// ============================================================
// 整数限幅
// ============================================================
int RADAR_PlotsCentroid::clampInt_(int x, int lo, int hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}
