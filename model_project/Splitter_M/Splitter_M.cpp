#include "Splitter_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Splitter_M )
{	
	SET_MODEL_DESCRIPTION("RF splitter for Envelope Matrix Signals");
	SET_MODEL_SYMBOL("SYM_Splitter_M");
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
		enumParam.SetDescription("Combination Mode: SubArray, Custom");
		enumParam.AddEnumeration("SubArray", SubArray);
		enumParam.AddEnumeration("Custom", Custom);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumRows);
		param.SetDescription("Number of rows for each element to split");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetHideCondition("Mode ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumCols);
		param.SetDescription("Number of columns for each element to split");
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

Splitter_M::Splitter_M()
{
	
}

bool Splitter_M::Setup()
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
bool Splitter_M::Run()
{
	inRow = input[0].NumRows();
	inCol = input[0].NumColumns();

	double InsertionLossM = std::pow(10, InsertionLoss / 10);

	switch (Mode)
	{
	case Splitter_M::SubArray:
		outRow = inRow * NumRows;
		outCol = inCol * NumCols;

		output[0].Resize(outRow, outCol);
		output[0].Zero();
		for (int m = 0; m < outRow; m++)
		{
			for (int n = 0; n < outCol; n++)
			{
				output[0](m, n) = input[0](m / NumRows, n / NumCols) / std::sqrt(InsertionLossM * NumRows * NumCols);
			}
		}
		break;

	case Splitter_M::Custom:
		numMap = ElementMap.NumElements();
		channelCount.Resize(input[0].NumElements(), 1);
		channelCount.Zero();

		// 检查映射图中元素是否有效，并对分配损耗序列进行计数
		for (int i = 0; i < numMap; i++)
		{
			if (ElementMap(i) < 1 || ElementMap(i) > input[0].NumElements())
			{
				POST_ERROR("The element in the ElementMap array must be an integer between 1 and the number of splitters.");
				return false;
			}

			channelCount(ElementMap(i) - 1)++; // 对应通道计数+1
		}

		output[0].Resize(numMap, 1);
		output[0].Zero();

		// 进行通道分配
		for (int i = 0; i < numMap; i++)
		{
			output[0](i) = input[0](ElementMap(i) - 1) / std::sqrt(InsertionLossM * channelCount(ElementMap(i) - 1));		
		}

		break;

	default:
		break;
	}

	return true;
}
