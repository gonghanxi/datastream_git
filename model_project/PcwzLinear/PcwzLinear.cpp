#include "PcwzLinear.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PcwzLinear )
{	
	SET_MODEL_DESCRIPTION("Piecewise Linear Mapper");
	SET_MODEL_SYMBOL("SYM_PcwzLinear");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Breakpoints);
		param.SetDescription("Endpoints and breakpoints in the mapping");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[-1 - j, j, 1 - j]");
		param.SetUseDefault(1);
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

PcwzLinear::PcwzLinear()
{
	
}

bool PcwzLinear::Setup()
{
	bool bStatus = true;

	numBreakpoints = Breakpoints.NumElements();
	if (numBreakpoints < 2)
	{
		POST_ERROR("Number of breakpoints must be larger than 1.");
		return false;
	}

	// 初始化斜率与截距
	slope.Resize(1, numBreakpoints - 1);
	intercept.Resize(1, numBreakpoints - 1);

	// 初始化当前端点
	currentBreakpoint = Breakpoints(0);
	
	// 每两个端点求斜率与截距
	for (int i = 1; i < numBreakpoints; i++)
	{
		x1 = currentBreakpoint.real();
		y1 = currentBreakpoint.imag();
		x2 = Breakpoints(i).real();
		y2 = Breakpoints(i).imag();

		if (x1 >= x2)
		{
			POST_ERROR("Breakpoints x (or real) values must be monotonically increasing.");
			return false;
		}

		slope(i) = (y2 - y1) / (x2 - x1);
		intercept(i) = (x2 * y1 - y2 * x1) / (x2 - x1);

		currentBreakpoint = Breakpoints(i);
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PcwzLinear::Run()
{
	// 横坐标依次递增遍历各端点，寻找输入值对应区间内的直线
	for (int i = 0; i < numBreakpoints; i++)
	{
		x1 = Breakpoints(i).real();
		y1 = Breakpoints(i).imag();
	
		if (input[0] < x1)
		{
			// 若输入小于第一个端点横坐标，则输出第一个端点的纵坐标
			// 否则正常按照当前端点对应的斜截式计算输入对应的输出值
			output[0] = i ? slope(i)*input[0] + intercept(i) : y1;
			return true;
		}
	}
	// 输入大于最后一个端点的横坐标，则输出最后一个端点的纵坐标
	output[0] = y1;

	return true;
}
