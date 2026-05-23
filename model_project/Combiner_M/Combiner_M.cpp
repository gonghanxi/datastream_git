#include "Combiner_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Combiner_M )
{	
	SET_MODEL_DESCRIPTION("RF combiner for Envelope Matrix Signals");
	SET_MODEL_SYMBOL("SYM_Combiner_M");
	SET_MODEL_CATEGORY("Beamforming");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Mode, SelectedMode);
		enumParam.SetDescription("Combination Mode: SubArray, Custom, FullArray");
		enumParam.AddEnumeration("SubArray", SubArray);
		enumParam.AddEnumeration("Custom", Custom);
		enumParam.AddEnumeration("FullArray", FullArray);
		enumParam.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumRows);
		param.SetDescription("Number of rows of the sub-matrix to be combined");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetHideCondition("Mode ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumCols);
		param.SetDescription("Number of columns of the sub-matrix to be combined");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetHideCondition("Mode ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(ElementMap);
		param.SetDescription("Custom mapping");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetHideCondition("Mode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InsertionLoss);
		param.SetDescription("Insertion loss");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

Combiner_M::Combiner_M()
{
	
}

bool Combiner_M::Setup()
{
	bool bStatus = true;

	if (NumRows < 1)
	{
		POST_ERROR("NumRows must be >= 1.");
		bStatus = false;
	}

	if (NumCols < 1)
	{
		POST_ERROR("NumCols must be >= 1.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Combiner_M::Run()
{
	inRow = input[0].NumRows();
	inCol = input[0].NumColumns();
	
	double InsertionLossM = std::pow(10, InsertionLoss / 10);

	switch (Mode)
	{
	case Combiner_M::SubArray:

		if (inRow % NumRows)
		{
			POST_ERROR("The input matrix row number inRow must be divisible by NumRows.");
			return false;
		}

		if (inCol % NumCols)
		{
			POST_ERROR("The input matrix column number inCol must be divisible by NumCols.");
			return false;
		}

		outRow = inRow / NumRows;
		outCol = inCol / NumCols;

		output[0].Resize(outRow, outCol);
		output[0].Zero();
		for (int m = 0; m < inRow; m++)
		{
			for (int n = 0; n < inCol; n++)
			{
				output[0](m / NumRows, n / NumCols) += input[0](m, n) / std::sqrt(InsertionLossM * NumRows * NumCols);
			}
		}
		break;

	case Combiner_M::Custom:
		numMap = ElementMap.NumElements();

		if (numMap != inRow * inCol)
		{
			POST_ERROR("The number of input signals does not match the size of the ElementMap array.");
			return false;
		}

		// 求出输出最大通道数
		maxChannel = 0;
		for (int i = 0; i < numMap; i++)
		{
			if (maxChannel < ElementMap(i))
			{
				maxChannel = ElementMap(i);
			}
		}

		channelCount.Resize(maxChannel, 1);
		channelCount.Zero();
		output[0].Resize(maxChannel, 1);
		output[0].Zero();

		// 进行通道合成
		for (int i = 0; i < numMap; i++)
		{
			if (ElementMap(i)) // 映射图中指向0的元素会被忽略
			{
				output[0](ElementMap(i) - 1) += input[0](i); 
				channelCount(ElementMap(i) - 1)++; // 对应通道计数+1
			}
		}

		// 根据合成损耗对输出进行加权
		for (int i = 0; i < maxChannel; i++)
		{
			output[0](i) /= std::sqrt(InsertionLossM * channelCount(i));
		}

		break;

	case Combiner_M::FullArray:
		output[0].Resize(1, 1);
		output[0].Zero();
		for (int m = 0; m < inRow; m++)
		{
			for (int n = 0; n < inCol; n++)
			{
				output[0](0, 0) += input[0](m, n) / std::sqrt(InsertionLossM * inRow * inCol);
			}
		}
		break;

	default:
		break;
	}
	return true;
}
