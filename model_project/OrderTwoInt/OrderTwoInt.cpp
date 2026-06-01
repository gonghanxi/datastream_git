#include "OrderTwoInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( OrderTwoInt )
{	
	SET_MODEL_DESCRIPTION("Ordered Two Integer Min/Max Function");
	SET_MODEL_SYMBOL("SYM_OrderTwoInt");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(upper);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(lower);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(greater);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(lesser);
	}
	return true;
}
#endif

OrderTwoInt::OrderTwoInt()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool OrderTwoInt::Run()
{
	greater[0] = std::max(upper[0], lower[0]);
	lesser[0] = std::min(upper[0], lower[0]);
	return true;
}
