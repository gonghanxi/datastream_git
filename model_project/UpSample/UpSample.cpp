#include "UpSample.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(UpSample)
{
	SET_MODEL_DESCRIPTION("Up Sampler for real signal");
	SET_MODEL_SYMBOL("SYM_UpSample");
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
		p.SetDescription("Number of samples produced");
	}

	// 参数：Mode（枚举）
	{
		SystemVueModelBuilder::DFParam e = ADD_MODEL_ENUM_PARAM(Mode, ModeEnum);
		e.SetUnit(SystemVueModelBuilder::Units::NONE);
		e.AddEnumeration("Insert zeros", Insertzeros);
		e.AddEnumeration("Hold sample", Holdsample);
		e.SetDefaultValue("Insert zeros");
		e.SetUseDefault(true);
		e.SetDescription("Mode method: Insert zeros, Hold sample");
		e.SetSchematicDisplay(false);
	}

	// 参数：Phase
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Phase);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Where to put the input in the output block. Visible when Mode mode is Insert zeros");
		p.SetHideCondition("Mode == 1 || Factor == 1");
		p.SetUseDefault(true);
		p.SetSchematicDisplay(false);
		p.SetDynamicUpdate(true);
	}
	return true;
}
#endif

UpSample::UpSample()
{
	m_bIsInRun = false;
}

bool UpSample::Setup()
{
	if (Factor < 1)
	{
		POST_ERROR("Factor must be >= 1");
		return false;
	}
	else
		output.SetRate(Factor);
	return true;
}

bool UpSample::Initialize()
{
	bool bStatus;
	
	if (Mode == ModeEnum::Insertzeros)
	{
		if (m_bIsInRun == false)
		{
			if ((Phase < 0) || (Factor > 0 && Phase >= Factor))
			{
				POST_ERROR("Phase must be > 0 and Phase must be < Factor");
				return false;
			}
		}
		else
		{
			if (Phase < 0)
				Phase = 0;
			else if (Factor > 0 && Phase >= Factor)
				Phase = Factor;
		}
		return true;
	}

	m_bIsInRun = true;
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool UpSample::Run()
{
	if (Mode == ModeEnum::Insertzeros)
	{
		output.Zero(0, 1, input.GetPointer(0));
	}
	else
	{
		input.Copy(0, &output, 0, 1);
	}

	// determine fill value.
	size_t i;
	for (i = 1; i < size_t(Factor); i++)
	{
		output.Copy(0, &output, i, 1);
	}

	if (Mode == ModeEnum::Insertzeros)
		input.Copy(0, &output, size_t(Phase), 1);

	return true;
}
