#include "DownSampleVarPhase.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( DownSampleVarPhase )
{	
	SET_MODEL_DESCRIPTION("DownSample with variable down sampling phase");
	SET_MODEL_SYMBOL("SYM_DownSampleVarPhase");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	// Add port
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(phase);
		port.SetName("phase");
		port.SetDescription("down sampling phase");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("output signal");
	}

	// ²ÎÊý£ºFactor
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Factor);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetName("Factor");
		p.SetDefaultValue("2");
		p.SetDescription("Downsample factor");
	}
	return true;
}
#endif

DownSampleVarPhase::DownSampleVarPhase()
{

}

bool DownSampleVarPhase::Setup()
{
	bool bStatus = true;

	if (Factor < 1)
	{
		POST_ERROR("Factor must be >= 1");
		bStatus = false;
	}
	else
	{
		input.SetRate(Factor);
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool DownSampleVarPhase::Run()
{
	int Phase = phase[0];
	if (Phase < 0)
	{
		Phase = 0;
	}
	if (Phase >= Factor)
	{
		Phase = Factor - 1;
	}

	output[0] = input[Phase];
	return true;
}
