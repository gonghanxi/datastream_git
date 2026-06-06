#pragma once
// SystemVue库头文件
#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"
#include "EnvelopeSignal.h"

#include <complex>

// RADAR_MTI_M类定义 - 动目标指示雷达矩阵版模型
class SYSTEMVUEMODELBUILDER_API RADAR_MTI_M : public SystemVueModelBuilder::DFModel
{
public:
	// 枚举类型 - MTI滤波器类型
	enum SelectedMTI_Type {
		TwoPulseCanceller = 0,   // 两脉冲对消器
		ThreePulseCanceller = 1  // 三脉冲对消器
	};

	// 必须的宏，用于定义SystemVue模型接口
	DECLARE_MODEL_INTERFACE(RADAR_MTI_M);

	// 构造函数
	RADAR_MTI_M();

	//-------- 函数重载 --------
	virtual bool Run();     // 主运行函数
	virtual bool Setup();   // 初始化函数

	// 端口定义
	// input  : complex matrix
	// output : complex matrix
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< std::complex<double> > > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< std::complex<double> > > output;

	// 参数定义
	SelectedMTI_Type MTI_Type;   // MTI滤波器类型
};
