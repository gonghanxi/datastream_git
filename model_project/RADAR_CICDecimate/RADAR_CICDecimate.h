#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <complex>
#include <vector>
#include <deque>

class SYSTEMVUEMODELBUILDER_API RADAR_CICDecimate : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_CICDecimate);

	RADAR_CICDecimate();

	virtual bool Setup();
	virtual bool Initialize();
	virtual bool Run();
	virtual bool Finalize();
	virtual bool UpdateDynamicParameters();

	typedef std::complex<double> Cx;

	// ============================================================
	// 端口定义
	// Port 1：input，待抽取的复数信号
	// Port 2：output，抽取后的复数信号
	// ============================================================
	SystemVueModelBuilder::DComplexCircularBuffer input;
	SystemVueModelBuilder::DComplexCircularBuffer output;

	// ============================================================
	// RADAR_CICDecimate 帮助文档参数
	// ============================================================
	int Order;       // CIC 级联阶数 N
	int Ratio;       // 抽取倍率 R，每 Ratio 个输入点输出 1 个点
	int DiffDelay;   // comb 部分差分延迟 M
	int Phase;       // 抽取相位，y[n] = x[Ratio*n + Phase]

private:
	int order_;
	int ratio_;
	int diffDelay_;
	int phase_;

	double gainScale_;

	// 高速积分器状态：Order 级，每个输入 token 更新一次
	std::vector<Cx> integratorState_;

	// 低速 comb 延迟线：Order 级，每级 DiffDelay 个低速历史样本
	std::vector< std::deque<Cx> > combDelay_;

private:
	bool validateAndPrepare_();
	void resetStates_();

	Cx runIntegratorStages_(const Cx& x);
	Cx runCombStages_(const Cx& x);

	double computeGainScale_() const;

	static int clampInt_(int x, int lo, int hi);
};
