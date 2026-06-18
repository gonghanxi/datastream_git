#include "ZeroCross.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( ZeroCross )
{	
	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );

	return true;
}
#endif

ZeroCross::ZeroCross()
{
	previousInput = 0;
	isCross = false;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool ZeroCross::Run()
{

	// ↓检测这个值就行了
	isCross = (previousInput*input[0] < 0);
	// ↑（OωO）

	previousInput = input[0];
	output[0] = input[0];
	
	return true;
}
