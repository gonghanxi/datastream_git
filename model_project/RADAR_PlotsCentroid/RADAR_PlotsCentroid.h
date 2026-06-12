#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <vector>

/*
 * RADAR_PlotsCentroid
 *
 * 功能：
 *   CFAR 后、BinaryDetector 前的点迹质心定心模块。
 *
 * 帮助文档要点：
 *   1. 输入 output 均为 real；
 *   2. Type 支持 1D / 2D；
 *   3. Type=1D 时只使用 SampleNum；
 *   4. Type=2D 时使用 RangeBinNum、DopplerBinNum；
 *   5. 2D 情况下 SampleNum 等价于 RangeBinNum * DopplerBinNum。
 *
 * 实现策略：
 *   - Type=1D：对连续 input[i] > 0 的点迹区间做幅度加权质心；
 *   - Type=2D：对 input > 0 的二维连通区域做幅度加权质心；
 *   - 每个点迹只在质心位置保留一个代表值，其余位置清零；
 *   - 代表值采用该点迹区域内最大值，便于后续 BinaryDetector 继续门限判决。
 */

class RADAR_PlotsCentroid : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_PlotsCentroid);

	RADAR_PlotsCentroid();

	virtual bool Setup();
	virtual bool Run();

	// ============================================================
	// Type 枚举
	// 帮助文档 / 参数界面：
	//   0: 1D
	//   1: 2D
	// ============================================================
	enum CentroidTypeEnum
	{
		Type_1D = 0,
		Type_2D = 1
	};

	// ============================================================
	// 端口
	// Port 1: input  - real，通常接 CFAR 输出
	// Port 2: output - real，输出定心后的 CFAR 结果
	// ============================================================
	SystemVueModelBuilder::CircularBuffer<double> input;
	SystemVueModelBuilder::CircularBuffer<double> output;

	// ============================================================
	// 参数
	// ============================================================
	CentroidTypeEnum Type;
	int SampleNum;
	int RangeBinNum;
	int DopplerBinNum;

private:
	int effectiveSampleNum_;

private:
	bool validateAndPrepare_();

	void run1D_();
	void run2D_();

	// 2D 展开方式：
	//   index = dopplerIndex * RangeBinNum + rangeIndex
	// 即同一个 Doppler bin 下，Range 方向连续排列。
	int  idx2D_(int rangeIndex, int dopplerIndex) const;
	bool isPositive_(double x) const;

	static int roundToNearestIndex_(double x);
	static int clampInt_(int x, int lo, int hi);
};
