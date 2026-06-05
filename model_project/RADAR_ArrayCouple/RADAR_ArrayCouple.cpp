#include "RADAR_ArrayCouple.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_ArrayCouple )
{	
	SET_MODEL_DESCRIPTION("This model is used to add couple effect of array antenna");

	SET_MODEL_CATEGORY("Array TR");

	ADD_MODEL_INPUT(input);

	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ChannelNum);
		param.SetDescription("The number of array antenna");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(CoupleCoef);
		param.SetDescription("The mutual coupling matrix of an antenna array, the dimension should be ChannelNum rows and ChannelNum columns.");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[1,0,0,0; 0,1,0,0; 0,0,1,0; 0,0,0,1]");
	}
	return true;
}
#endif

RADAR_ArrayCouple::RADAR_ArrayCouple()
{
	
}

bool RADAR_ArrayCouple::Setup()
{
	bool bStatue = true;

	if (CoupleCoef.NumColumns() != ChannelNum || CoupleCoef.NumRows() != ChannelNum)
	{
		POST_ERROR("Columns and Rows of CoupleCoef must = ChannelNum");
		bStatue = false;
	}

	return bStatue;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_ArrayCouple::Run()
{
	for (int m = 0; m < ChannelNum; m++)
	{
		// 输出通道初始化
		output[m][0] = 0.0;

		// 与耦合效应矩阵相乘
		for (int n = 0; n < ChannelNum; n++)
		{
			output[m][0] += input[n][0] * CoupleCoef(m, n);
		}
	}

	return true;
}
