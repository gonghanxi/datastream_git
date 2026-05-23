#include "VarDelayEnv.h"

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(VarDelayEnv)
{
	SET_MODEL_DESCRIPTION("Variable Delay (Envelope)");
	SET_MODEL_SYMBOL("SYM_VarDelay");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("input signal");
	}

	{
		DFPort p = ADD_MODEL_INPUT(control);
		p.SetDescription("control signal");
		p.SetOptional(true);
	}

	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output signal");
	}

	{
		DFParam param = ADD_MODEL_PARAM(MaxDelay);
		param.SetUnit(Units::NONE);
		param.SetDefaultValue("10");
		param.SetDescription("Maximum delay");
	}

	return true;
}
#endif

VarDelayEnv::VarDelayEnv()
	: MaxDelay(10),
	m_iDelay(0),
	m_iMaxDelay(0)
{
}

bool VarDelayEnv::Setup()
{
	if (MaxDelay < 0)
	{
		POST_ERROR("VarDelayEnv: MaxDelay must be >= 0.");
		return false;
	}

	m_iMaxDelay = static_cast<size_t>(MaxDelay);
	m_iDelay = m_iMaxDelay;

	m_buffer.ResizeMemory(m_iMaxDelay + 1, true, 0);
	m_buffer.SetHistoryDepth(m_iMaxDelay + 1);

	return true;
}

bool VarDelayEnv::Run()
{
	if (control.IsConnected())
	{
		int ctrlVal = control[0];

		if (ctrlVal <= 0)
		{
			m_iDelay = 0;
		}
		else
		{
			m_iDelay = static_cast<size_t>(ctrlVal);
			if (m_iDelay > m_iMaxDelay)
				m_iDelay = m_iMaxDelay;
		}
	}

	m_buffer[m_iMaxDelay] = input[0];

	output[0] = m_buffer[m_iMaxDelay - m_iDelay];

	m_buffer.Advance();

	return true;
}
