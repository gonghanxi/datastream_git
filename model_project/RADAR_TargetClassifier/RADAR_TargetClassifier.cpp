#include "RADAR_TargetClassifier.h"
#include <vector>
#include <limits>
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_TargetClassifier )
{	
	SET_MODEL_DESCRIPTION("Radar target classifier");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(trainIn);
		port.SetDescription("Input training data.");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(predictIn);
		port.SetDescription("Input data to predict.");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(predictOut);
		port.SetDescription("Output predict result.");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(centroid);
		port.SetDescription("Output centroid of each type.");
	}

	
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ClassifierType, SelectedClassifierType);
		enumParam.SetDescription("Classifier type");
		enumParam.AddEnumeration("Kmeans", Kmeans);
		enumParam.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(K);
		param.SetDescription("Num of target types K for K-means");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TrainSize);
		param.SetDescription("Sample num to train the classifier each run");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PredictSize);
		param.SetDescription("Sample num to predict each run");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(MaxIteration);
		param.SetDescription("Max iteration num for clustering");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("200");
	}

	return true;
}
#endif

RADAR_TargetClassifier::RADAR_TargetClassifier()
{
	
}

bool RADAR_TargetClassifier::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (K < 2)
	{
		POST_ERROR("K must be >= 2");
		bStatus = false;
	}
	if (K > TrainSize)
	{
		POST_ERROR("K must be <= TrainSize");
		bStatus = false;
	}
	if (TrainSize <= 0)
	{
		POST_ERROR("TrainSize must be > 0");
		bStatus = false;
	}
	if (PredictSize <= 0)
	{
		POST_ERROR("PredictSize must be > 0");
		bStatus = false;
	}

	if (bStatus)
	{
		trainIn.SetRate(TrainSize);
		predictIn.SetRate(PredictSize);
		predictOut.SetRate(PredictSize);
		centroid.SetRate(K);
	}

	return bStatus;
}


//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_TargetClassifier::Run()
{
	// ————————————————————————————————————————
	// 训练阶段
	// ————————————————————————————————————————
	std::vector<int> labels(TrainSize);

	// 初始化：随机选择k个数据点作为初始质心
	std::mt19937 rng(42);
	std::vector<int> idx(TrainSize);
	for (int i = 0; i < TrainSize; ++i) idx[i] = i;
	std::shuffle(idx.begin(), idx.end(), rng);
	for (int i = 0; i < K; ++i) centroid[i] = trainIn[idx[i]];

	for (int iter = 0; iter < MaxIteration; ++iter)
	{
		// 分配：每个点归入最近的质心
		bool changed = false;
		for (int i = 0; i < TrainSize; ++i)
		{
			double bestDist = std::numeric_limits<double>::max();
			int bestIdx = 0;
			for (int j = 0; j < K; ++j)
			{
				// 计算样点到每个质心的距离
				double dist = std::abs(trainIn[i] - centroid[j]);
				if (dist < bestDist)
				{
					bestDist = dist;
					bestIdx = j;
				}
				if (labels[i] != bestIdx)
				{
					labels[i] = bestIdx;
					changed = true;
				}
			}
			// 质心不发生变化时提前结束迭代
			if (!changed) break;

			// 更新：重新计算质心
			for (int j = 0; j < K; ++j)
			{
				centroid[j] = 0;
				int cnt = 0;
				for (int i = 0; i < TrainSize; ++i)
				{
					if (labels[i] != j) continue;
					centroid[j] += trainIn[j];
					++cnt;
				}
				if (cnt > 0)
				{
					centroid[j] /= cnt;
				}
			}
		}
	}

	// ————————————————————————————————————————
	// 预测阶段
	// ————————————————————————————————————————
	for (int i = 0; i < PredictSize; ++i)
	{
		double bestDist = std::numeric_limits<double>::max();
		for (int j = 0; j < K; ++j)
		{
			// 计算样点到各个质心的距离
			double dist = std::abs(predictIn[i] - centroid[j]);
			if (dist < bestDist)
			{
				bestDist = dist;
				// 距离最小的质心即待测样点的类别
				predictOut[i] = j;
			}
		}
	}

	return true;
}


