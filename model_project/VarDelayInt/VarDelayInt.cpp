#include "VarDelayInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(VarDelayInt)
{
	using namespace SystemVueModelBuilder;

	SET_MODEL_DESCRIPTION("Variable Delay");
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
		DFParam p = ADD_MODEL_PARAM(MaxDelay);
		p.SetUnit(Units::NONE);
		p.SetDefaultValue("10");
		p.SetDescription("Maximum delay");
	}

	return true;
}
#endif 

using namespace SystemVueModelBuilder;

VarDelayInt::VarDelayInt()
	: MaxDelay(10),
	m_iDelay(0),
	m_iMaxDelay(0)
{
}

bool VarDelayInt::Setup()
{
	if (MaxDelay < 0)
	{
		POST_ERROR("VarDelayInt: MaxDelay must be >= 0");
		return false;
	}

	m_iMaxDelay = static_cast<size_t>(MaxDelay);
	m_iDelay = m_iMaxDelay;          

	m_buffer.ResizeMemory(m_iMaxDelay + 1, true, 0);
	m_buffer.SetHistoryDepth(m_iMaxDelay + 1);

	return true;
}

bool VarDelayInt::Run()
{
	if (control.IsConnected())
	{
		int c = control[0];

		if (c < 0)
			c = 0;
		else if (c > MaxDelay)
			c = MaxDelay;

		m_iDelay = static_cast<size_t>(c);
	}

	m_buffer[m_iMaxDelay] = input[0];

	output[0] = m_buffer[m_iMaxDelay - m_iDelay];

	m_buffer.Advance();

	return true;
}
