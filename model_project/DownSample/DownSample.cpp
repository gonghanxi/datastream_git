#include "DownSample.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( DownSample )
{	
	SET_MODEL_DESCRIPTION("Downsampler");
	SET_MODEL_SYMBOL("SYM_DownSample");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	// Add port
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("output signal");
	}

	// 参数：Factor
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Factor);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetName("Factor");
		p.SetDefaultValue("2");
		p.SetDescription("Downsample factor");
	}

	// 参数：Phase
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Phase);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Downsample phase");
		p.SetUseDefault(true);
		p.SetSchematicDisplay(false);
		p.SetDynamicUpdate(true);
	}
	return true;
}
#endif

DownSample::DownSample()
{

}

bool DownSample::Setup()
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

	if (Phase < 0 || Phase >= Factor)
	{
		POST_ERROR("Phase must be >= 1 and < Factor");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool DownSample::Run()
{
	output[0] = input[Phase];
	return true;
}
