#include "RADAR_MTI.h"

// 以下代码仅在非代码生成环境中编译
#ifndef SV_CODE_GEN
// 定义SystemVue模型接口
DEFINE_MODEL_INTERFACE(RADAR_MTI)
{
	SET_MODEL_DESCRIPTION("Moving Target Indication");
	SET_MODEL_SYMBOL("SYM_RADAR_MTI@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// 添加输入端口
	ADD_MODEL_INPUT(input);
	// 添加输出端口
	ADD_MODEL_OUTPUT(output);

	// 添加PRI（脉冲重复间隔）参数
	SystemVueModelBuilder::DFParam P2 = ADD_MODEL_PARAMETER(PRI);
	P2.SetUnit(SystemVueModelBuilder::Units::TIME);		// 设置单位为时间
	P2.SetDefaultValue("1e-4");						// 默认值0.1ms

	// 添加NumOfPulse（脉冲数量）参数
	SystemVueModelBuilder::DFParam P3 = ADD_MODEL_PARAMETER(NumOfPulse);
	P3.SetDefaultValue("32");						// 默认32个脉冲

	// 添加MTI滤波器类型枚举参数
	SystemVueModelBuilder::DFParam P4 = ADD_MODEL_ENUM_PARAMETER(MTI_Type, SelectedMTI_Type);
	P4.AddEnumeration("Two Pulse Canceller", TwoPulseCanceller);		// 两脉冲对消
	P4.AddEnumeration("Three Pulse Canceller", ThreePulseCanceller);	// 三脉冲对消
	P4.SetDefaultValue(0);		// 默认使用两脉冲对消

	// 添加采样率参数
	SystemVueModelBuilder::DFParam P6 = ADD_MODEL_PARAMETER(SampleRate);
	P6.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);	// 设置单位为频率
	P6.SetDefaultValue("10e6");					// 默认10MHz采样率

	return true;	// 接口定义成功
}
#endif

// 构造函数
RADAR_MTI::RADAR_MTI() : samplesPerPulse(0)
{
	// 初始化samplesPerPulse为0
}

// 初始化函数
bool RADAR_MTI::Setup()
{
	// 计算单脉冲的采样点数：PRI * SampleRate
	samplesPerPulse = static_cast<int>(round(PRI * SampleRate));

	// 计算输入信号总采样点数 = 脉冲数 × 单脉冲采样点数
	int inputTotalSamples = NumOfPulse * samplesPerPulse;

	// 设置输入缓冲区大小
	input.SetRate(inputTotalSamples);

	// 根据MTI类型计算输出脉冲数
	int outputPulseNum = 0;
	if (MTI_Type == TwoPulseCanceller) {
		// 两脉冲对消：输出脉冲数 = 输入脉冲数 - 1
		outputPulseNum = (NumOfPulse - 1);
	}
	else if(MTI_Type == ThreePulseCanceller) {
		// 三脉冲对消：输出脉冲数 = 输入脉冲数 - 2
		outputPulseNum = (NumOfPulse - 2);
	}

	// 计算输出信号总采样点数
	int outputTotalSamples = outputPulseNum * samplesPerPulse;

	// 设置输出缓冲区大小
	output.SetRate(outputTotalSamples);

	return true;	// 初始化成功
}

//-----------------------------------------------------------------------------------
//	Run - 主运行函数
//		在这里进行MTI算法计算
//-----------------------------------------------------------------------------------
bool RADAR_MTI::Run()
{
	// 检查脉冲数量是否满足算法要求
	if ((MTI_Type == TwoPulseCanceller && NumOfPulse < 2) ||
		(MTI_Type == ThreePulseCanceller && NumOfPulse < 3))
	{
		return false; // 脉冲数不足，返回错误
	}

	// 获取输入输出缓冲区大小
	int inputSize = input.GetSize();
	int outputSize = output.GetSize();

	// 重新计算输入输出总采样点数
	int inputTotalSamples = NumOfPulse * samplesPerPulse;
	int outputPulseNum = (MTI_Type == TwoPulseCanceller) ? (NumOfPulse - 1) : (NumOfPulse - 2);
	int outputTotalSamples = outputPulseNum * samplesPerPulse;

	if (inputSize < inputTotalSamples || outputSize < outputTotalSamples)
	{
		return false; // 输入数据不足或输出缓冲区无效
	}

	// 根据MTI滤波器类型选择相应的算法
	switch (MTI_Type)
	{
	case TwoPulseCanceller:
		// 两脉冲对消：y[n] = x[n] - x[n-1]
		// 对每个脉冲对进行对消处理
		for (int pulse = 1; pulse < NumOfPulse; ++pulse)
		{
			for (int sample = 0; sample < samplesPerPulse; ++sample)
			{
				// 计算输入数据的索引
				int inputIdxPrev = (pulse - 1) * samplesPerPulse + sample;	// 前一个脉冲
				int inputIdxCurr = pulse * samplesPerPulse + sample;		// 当前脉冲
				int outputIdx = (pulse - 1) * samplesPerPulse + sample;	// 输出索引

				// 执行对消：当前脉冲减去前一个脉冲
				output[outputIdx] = input[inputIdxCurr] - input[inputIdxPrev];
			}
		}
		break;

	case ThreePulseCanceller:
		// 三脉冲对消：y[n] = x[n] - 2*x[n-1] + x[n-2]
		// 对每个脉冲三元组进行对消处理
		for (int pulse = 2; pulse < NumOfPulse; ++pulse)
		{
			for (int sample = 0; sample < samplesPerPulse; ++sample)
			{
				// 计算输入数据的索引
				int inputIdxPrev2 = (pulse - 2) * samplesPerPulse + sample;	// 前两个脉冲
				int inputIdxPrev1 = (pulse - 1) * samplesPerPulse + sample;	// 前一个脉冲
				int inputIdxCurr = pulse * samplesPerPulse + sample;		// 当前脉冲
				int outputIdx = (pulse - 2) * samplesPerPulse + sample;	// 输出索引

				// 执行对消：当前脉冲 - 2×前一个脉冲 + 前两个脉冲
				output[outputIdx] = input[inputIdxCurr] - input[inputIdxPrev1] * 2.0 + input[inputIdxPrev2];
			}
		}
		break;
	}

	return true;	// 运行成功
}

