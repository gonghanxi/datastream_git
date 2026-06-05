#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

class SYSTEMVUEMODELBUILDER_API RADAR_CFAR_M : public SystemVueModelBuilder::DFModel
{
public:
	// ===== 枚举：顺序和帮助文档保持一致 =====
	// RADAR_CFAR_M 帮助文档只列出 CA / SOCA / GOCA / Clutter Map，不包含普通版 RADAR_CFAR 的 OS。
	enum SelectedCFARType { CA = 0, SOCA = 1, GOCA = 2, ClutterMap = 3 };
	enum SelectedCFARDimension { Range = 0, Doppler = 1 };
	enum SelectedDetectorType { Envelope = 0, Square = 1, LogSquare = 2, Log = 3 };
	enum SelectedThresholdType { ThresholdByPf = 0, ThresholdByAlpha = 1 };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(RADAR_CFAR_M);

	// Constructor to initialize parameters
	RADAR_CFAR_M();

	//-------- Function Overloads --------
	virtual bool Setup();
	virtual bool Run();

	// ===== 普通版 RADAR_CFAR 中已经使用过的门限因子辅助函数 =====
	double SOFactor(double alpha, int N);
	double GOFactor(double alpha, int N);
	double ClutterMapPointFactor(double alpha, int m, double r);
	double ClutterMapPlaneFactor(double alpha, int m, double r, double M);
	double SolutionBinaray(double pf, int N, double a, double b, double precision, SelectedCFARType cfarType);

	// ===== 端口 =====
	// input     : real matrix
	// output    : real matrix
	// threshold : real matrix
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > output;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > threshold;

	// ===== 参数：严格按 RADAR_CFAR_M 帮助文档列出 =====
	SelectedCFARType      CFAR_Type;
	SelectedCFARDimension CFAR_Dimension;
	int                   CellSize;
	int                   ReferenceCell;
	int                   GuardCell;
	SelectedDetectorType  Detector_Type;
	SelectedThresholdType Threshold;
	double                Pf;
	double                Alpha;
	double                Beta;

	double                ThresholdFactor;

private:
	// ===== Clutter Map 跨帧状态 =====
	bool clutterMapInitialized_;
	SystemVueModelBuilder::Matrix<double> clutterMap_;

private:
	// 矩阵尺寸访问：SystemVue 2020 Matrix 常用 NumRows() / NumColumns()
	int numRows(const SystemVueModelBuilder::Matrix<double>& m) const;
	int numCols(const SystemVueModelBuilder::Matrix<double>& m) const;

	// 检波器：矩阵版输入是 real matrix，因此这里按实数视频信号处理
	double detectorLaw(double x) const;

	// 门限因子：Threshold=Pf 时由 Pf 估计；Threshold=Alpha 时直接使用 Alpha
	double computeThresholdFactor() const;

	// 处理一条一维序列
	void processOneVector(
		const std::vector<double>& rawLine,
		std::vector<double>& outLine,
		std::vector<double>& thLine,
		std::vector<double>* clutterLine);

	// Clutter Map 处理一条一维序列
	void processOneVectorClutterMap(
		const std::vector<double>& rawLine,
		std::vector<double>& outLine,
		std::vector<double>& thLine,
		std::vector<double>& clutterLine);
};
