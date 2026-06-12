#pragma once

#include "SystemVue.h"
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <complex>
#include <vector>

class SYSTEMVUEMODELBUILDER_API RADAR_CICInterp : public SystemVueModelBuilder::DFModel
{
public:
	typedef std::complex<double> Cx;

	DECLARE_MODEL_INTERFACE(RADAR_CICInterp);

	RADAR_CICInterp();

	virtual bool Setup();
	virtual bool Initialize();
	virtual bool Run();
	virtual bool Finalize();

	// ============================================================
	// 端口定义
	// Port 1：input，complex，待内插的复数输入信号
	// Port 2：output，complex，内插后的复数输出信号
	// ============================================================
	SystemVueModelBuilder::DComplexCircularBuffer input;
	SystemVueModelBuilder::DComplexCircularBuffer output;

	// ============================================================
	// RADAR_CICInterp 帮助文档参数
	// ============================================================
	int Order;       // CIC 级联阶数 N
	int Ratio;       // 内插倍率 R，每输入 1 点输出 R 点
	int DiffDelay;   // comb 差分延迟 M
	int Phase;       // specified-stuffer 中输入点所在位置，范围 0 ~ Ratio-1
	Cx  Fill;        // specified-stuffer 中非输入位置的填充值

private:
	int order_;      // 经过边界检查后的 Order
	int ratio_;      // 经过边界检查后的 Ratio
	int diffDelay_;  // 经过边界检查后的 DiffDelay
	int phase_;      // 经过边界检查后的 Phase
	double gainScale_; // CIC 内插直流增益归一化系数，内置模块默认输出已归一化

	// 每一级 comb 的低速侧延迟线，尺寸为 order_ × diffDelay_
	std::vector< std::vector<Cx> > combDelay_;
	std::vector<int> combWriteIndex_;

	// 每一级 integrator 的高速侧累加状态，尺寸为 order_
	std::vector<Cx> integratorState_;

private:
	bool validateParameters_();
	void resetStates_();
	void updateGainScale_();

	Cx runCombStages_(const Cx& x);
	Cx runIntegratorStages_(const Cx& x);
};
