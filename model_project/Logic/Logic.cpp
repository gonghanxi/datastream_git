#include "Logic.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Logic )
{	
	SET_MODEL_DESCRIPTION("Boolean Logic Function");
	SET_MODEL_SYMBOL("SYM_+#Logic+");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(LogicOperation, SelectedLogicOperation);
		enumParam.SetName("Logic");
		enumParam.SetDescription("Logic operation: NOT, AND, NAND, OR, NOR, XOR, XNOR");
		enumParam.AddEnumeration("NOT", NOT);
		enumParam.AddEnumeration("AND", AND);
		enumParam.AddEnumeration("NAND", NAND);
		enumParam.AddEnumeration("OR", OR);
		enumParam.AddEnumeration("NOR", NOR);
		enumParam.AddEnumeration("XOR", XOR);
		enumParam.AddEnumeration("XNOR", XNOR);
		enumParam.SetDefaultValue("1");
	}
	return true;
}
#endif

Logic::Logic()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Logic::Run()
{
	bool bStatus = true;
	int	ChannelNumIn = input.GetSize();

	output[0] = input[0][0]; // 取第一个输入值初始化

	switch (LogicOperation)
	{
	case Logic::NOT:
		if (ChannelNumIn != 1)
		{
			POST_ERROR("NOT operatuon can only have one input.");
			bStatus = false;
		}
		output[0] = !(input[0][0]);
		break;
	case Logic::AND:
		for (int i = 1; i < ChannelNumIn; i++)
		{
			output[0] = output[0] && input[i][0];
		}
		break;
	case Logic::NAND:
		for (int i = 1; i < ChannelNumIn; i++)
		{
			output[0] = output[0] && input[i][0];
		}
		output[0] = !(output[0]);
		break;
	case Logic::OR:
		for (int i = 1; i < ChannelNumIn; i++)
		{
			output[0] = output[0] || input[i][0];
		}
		break;
	case Logic::NOR:
		for (int i = 1; i < ChannelNumIn; i++)
		{
			output[0] = output[0] || input[i][0];
		}
		output[0] = !(output[0]);
		break;
	case Logic::XOR:
		for (int i = 1; i < ChannelNumIn; i++)
		{
			output[0] = output[0] ^ input[i][0];
		}
		break;
	case Logic::XNOR:
		for (int i = 1; i < ChannelNumIn; i++)
		{
			output[0] = output[0] ^ input[i][0];
		}
		output[0] = !(output[0]);
		break;
	default:
		break;
	}
	return bStatus;
}
