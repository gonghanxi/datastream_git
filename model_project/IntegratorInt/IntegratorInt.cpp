#include "IntegratorInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(IntegratorInt)
{
	SET_MODEL_DESCRIPTION("Integer Integrator with Reset");
	SET_MODEL_SYMBOL("SYM_Integrator");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		auto p = ADD_MODEL_INPUT(reset);
		p.SetDescription("reset");
		p.SetOptional(true); 
	}
	{
		auto p = ADD_MODEL_INPUT(data);
		p.SetDescription("input");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(LimitOutput, LimitOutputEnum);
		p.AddEnumeration("No", IntegratorInt::LIMIT_NO);
		p.AddEnumeration("Saturate", IntegratorInt::LIMIT_SATURATE);
		p.AddEnumeration("Wrap", IntegratorInt::LIMIT_WRAP);
		p.SetDefaultValue("No");
		p.SetDescription("Output limiter options");
	}

	{
		auto p = ADD_MODEL_PARAM(Top);
		p.SetDefaultValue("0");
		p.SetDescription("Upper integer limit. Visible when LimitOutput is enabled.");
		p.SetHideCondition("LimitOutput == 0");
	}
	{
		auto p = ADD_MODEL_PARAM(Bottom);
		p.SetDefaultValue("0");
		p.SetDescription("Lower integer limit. Visible when LimitOutput is enabled.");
		p.SetHideCondition("LimitOutput == 0");
	}

	{
		auto p = ADD_MODEL_PARAM(InitialState);
		p.SetDefaultValue("0");
		p.SetDescription("Initial integrator state");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(UseIntegrationWindow, WindowEnum);
		p.AddEnumeration("No", IntegratorInt::WIN_NO);
		p.AddEnumeration("DefinedInTime", IntegratorInt::WIN_DEFINED_IN_TIME);
		p.AddEnumeration("DefinedInSamples", IntegratorInt::WIN_DEFINED_IN_SAMPLES);
		p.SetDefaultValue("No");
		p.SetDescription("Enable integration window");
	}

	{
		auto p = ADD_MODEL_PARAM(IntegrationTime);
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("100e-6");
		p.SetDescription("Integration time. Visible when UseIntegrationWindow is DefinedInTime.");
		p.SetHideCondition("UseIntegrationWindow ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(IntegrationSamples);
		p.SetDefaultValue("100");
		p.SetDescription("Integration samples. Visible when UseIntegrationWindow is DefinedInSamples.");
		p.SetHideCondition("UseIntegrationWindow ~= 2");
	}

	return true;
}
#endif 

IntegratorInt::IntegratorInt()
	: LimitOutput(LIMIT_NO)
	, Top(0)
	, Bottom(0)
	, InitialState(0)
	, UseIntegrationWindow(WIN_NO)
	, IntegrationTime(100e-6)
	, IntegrationSamples(100)
	, state_(0)
	, haveState_(false)
	, resetConnected_(false)
{
}

bool IntegratorInt::Setup()
{
	resetConnected_ = reset.IsConnected();

	state_ = 0;
	haveState_ = false;
	valueWindow_.clear();
	timeWindow_.clear();

	if (data.GetSampleRate() > 0.0)
		output.SetSampleRate(data.GetSampleRate());

	return true;
}

void IntegratorInt::applyLimits()
{
	if (LimitOutput == LIMIT_NO)
		return;

	if (Top < Bottom)
		std::swap(Top, Bottom);

	long long y = state_;

	if (LimitOutput == LIMIT_SATURATE)
	{
		if (y > Top)    y = Top;
		if (y < Bottom) y = Bottom;
		state_ = y;
	}
	else if (LimitOutput == LIMIT_WRAP)
	{
		const long long span = static_cast<long long>(Top) - static_cast<long long>(Bottom) + 1;
		if (span <= 0)
			return;

		long long offset = (y - Bottom) % span;
		if (offset < 0)
			offset += span;
		state_ = Bottom + offset;
	}
}

bool IntegratorInt::Run()
{
	const double t = data.GetTime(0, GetCount());
	(void)t; 

	const int  x = data[0];
	const bool rActive = resetConnected_ && (reset[0] != 0);
	const bool firstOut = !haveState_;

	if (UseIntegrationWindow == WIN_NO)
	{
		if (firstOut)
		{
			if (rActive)
			{
				state_ = x;
			}
			else
			{
				state_ = static_cast<long long>(InitialState) + x;
			}
		}
		else
		{
			if (rActive)
			{
				state_ = x;
			}
			else
			{
				state_ += x;
			}
		}
	}
	else
	{
		if (firstOut)
		{
			if (rActive)
			{
				state_ = x;
			}
			else
			{
				state_ = static_cast<long long>(InitialState) + x;
			}
			valueWindow_.clear();
			timeWindow_.clear();
			valueWindow_.push_back(x);
			timeWindow_.push_back(t);
		}
		else
		{
			if (rActive)
			{
				state_ = x;
				valueWindow_.clear();
				timeWindow_.clear();
				valueWindow_.push_back(x);
				timeWindow_.push_back(t);
			}
			else
			{
				state_ += x;
				valueWindow_.push_back(x);
				timeWindow_.push_back(t);

				if (UseIntegrationWindow == WIN_DEFINED_IN_TIME)
				{
					while (!valueWindow_.empty() &&
						(t - timeWindow_.front()) > IntegrationTime)
					{
						state_ -= valueWindow_.front();
						valueWindow_.pop_front();
						timeWindow_.pop_front();
					}
				}
				else if (UseIntegrationWindow == WIN_DEFINED_IN_SAMPLES)
				{
					while ((int)valueWindow_.size() > IntegrationSamples)
					{
						state_ -= valueWindow_.front();
						valueWindow_.pop_front();
						timeWindow_.pop_front();
					}
				}
			}
		}
	}

	haveState_ = true;

	applyLimits();

	output[0] = static_cast<int>(state_);
	return true;
}
