#include "SlidWinAvg.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SlidWinAvg )
{	
	SET_MODEL_DESCRIPTION("Sliding-Window Averager");
	SET_MODEL_SYMBOL("SYM_SlidWinAvg");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(WindowSize);
		param.SetDescription("Size of sliding window");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("3");
	}
	return true;
}
#endif

SlidWinAvg::SlidWinAvg()
{
	currentIndex = 0;
	currentSum = 0.0;
}

bool SlidWinAvg::Setup()
{
	bool bStatus = true;
	if (WindowSize > 0)
	{
		slideWindow.Resize(1, WindowSize);
		slideWindow.Zero();
	}
	else
	{
		POST_ERROR("WindowSize must > 0");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SlidWinAvg::Run()
{
	currentSum += input[0] - slideWindow(currentIndex);	// 移动滑窗，即减去最旧的值并加上新的输入值，求新滑窗内的和
	slideWindow(currentIndex) = input[0];				// 滑窗最旧的值更新为新输入的值
	currentIndex = (currentIndex + 1) % WindowSize;		// 更新指针（索引）
	output[0] = currentSum / WindowSize;

	return true;
}
