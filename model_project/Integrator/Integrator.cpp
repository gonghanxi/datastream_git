#include "Integrator.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Integrator)
{
	SET_MODEL_DESCRIPTION("Integrator with Reset");
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
		auto p = ADD_MODEL_ENUM_PARAM(IntegrationMethod, IntegrationMethodEnum);
		p.AddEnumeration("Rectangle", Integrator::RECTANGLE);
		p.AddEnumeration("Trapezoidal", Integrator::TRAPEZOIDAL);
		p.SetDefaultValue("Rectangle");
		p.SetDescription("Integration method");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(LimitOutput, LimitOutputEnum);
		p.AddEnumeration("No", Integrator::LIMIT_NO);
		p.AddEnumeration("Saturate", Integrator::LIMIT_SATURATE);
		p.AddEnumeration("Wrap", Integrator::LIMIT_WRAP);
		p.SetDefaultValue("No");
		p.SetDescription("Output limiter options");
	}

	{
		auto p = ADD_MODEL_PARAM(Top);
		p.SetDefaultValue("0");
		p.SetDescription("Upper limit. Visible when LimitOutput is enabled.");
		p.SetHideCondition("LimitOutput == 0");
	}
	{
		auto p = ADD_MODEL_PARAM(Bottom);
		p.SetDefaultValue("0");
		p.SetDescription("Lower limit. Visible when LimitOutput is enabled.");
		p.SetHideCondition("LimitOutput == 0");
	}

	{
		auto p = ADD_MODEL_PARAM(InitialState);
		p.SetDefaultValue("0");
		p.SetDescription("Initial integrator state");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(UseIntegrationWindow, WindowEnum);
		p.AddEnumeration("No", Integrator::WIN_NO);
		p.AddEnumeration("DefinedInTime", Integrator::WIN_DEFINED_IN_TIME);
		p.AddEnumeration("DefinedInSamples", Integrator::WIN_DEFINED_IN_SAMPLES);
		p.SetDefaultValue("No");
		p.SetDescription("Enable integration window");
	}

	{
		auto p = ADD_MODEL_PARAM(FeedbackGain);
		p.SetDefaultValue("1");
		p.SetDescription("Gain on feedback path, Visible when UseIntegrationWindow is disabled.");
		p.SetHideCondition("UseIntegrationWindow ~= 0");
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

Integrator::Integrator()
	: IntegrationMethod(RECTANGLE),
	LimitOutput(LIMIT_NO),
	Top(0.0),
	Bottom(0.0),
	InitialState(0.0),
	UseIntegrationWindow(WIN_NO),
	FeedbackGain(1.0),
	IntegrationTime(100e-6),
	IntegrationSamples(100),
	Ts_(0.0),
	lastTime_(0.0),
	haveLastTime_(false),
	state_(0.0),
	prevInput_(0.0),
	resetConnected_(false)
{
}

bool Integrator::Setup()
{
	resetConnected_ = reset.IsConnected();

	Ts_ = 0.0;
	haveLastTime_ = false;
	lastTime_ = 0.0;

	state_ = InitialState;
	prevInput_ = 0.0;

	areaWindow_.clear();
	timeWindow_.clear();

	if (data.GetSampleRate() > 0.0)
		output.SetSampleRate(data.GetSampleRate());

	return true;
}

double Integrator::computeArea(double xPrev, double xCurr, double Ts)
{
	if (Ts <= 0.0)
		return 0.0;

	if (IntegrationMethod == RECTANGLE)
	{
		return Ts * xCurr;
	}
	else 
	{
		return 0.5 * Ts * (xPrev + xCurr);
	}
}

void Integrator::applyLimits()
{
	if (LimitOutput == LIMIT_NO)
		return;

	if (Top < Bottom)
		std::swap(Top, Bottom);

	if (LimitOutput == LIMIT_SATURATE)
	{
		if (state_ > Top)    state_ = Top;
		if (state_ < Bottom) state_ = Bottom;
	}
	else if (LimitOutput == LIMIT_WRAP)
	{
		const double span = Top - Bottom;
		if (span <= 0.0)
			return;

		state_ = Bottom + std::fmod(state_ - Bottom, span);
		if (state_ < Bottom)
			state_ += span;
	}
}

bool Integrator::Run()
{
	const double t = data.GetTime(0, GetCount());
	const double x = data[0];
	const bool   rActive = resetConnected_ && (reset[0] != 0);

	const bool firstSample = !haveLastTime_;

	double Ts = 0.0;
	if (firstSample)
	{
		if (data.GetSampleRate() > 0.0)
			Ts = 1.0 / data.GetSampleRate();
		else
			Ts = Ts_;   
	}
	else
	{
		Ts = t - lastTime_;
		if (Ts <= 0.0 && Ts_ > 0.0)
			Ts = Ts_;
	}

	if (Ts > 0.0)
		Ts_ = Ts;

	double area;
	if (IntegrationMethod == RECTANGLE)
	{
		area = Ts * x;
	}
	else
	{
		const double xPrev = firstSample ? x : prevInput_;
		area = 0.5 * Ts * (xPrev + x);
	}


	if (UseIntegrationWindow == WIN_NO)
	{
		if (firstSample)
		{
			if (rActive)
			{
				state_ = area;
			}
			else
			{
				state_ = InitialState;
			}
		}
		else
		{
			if (rActive)
			{
				state_ = area;
			}
			else
			{
				state_ = FeedbackGain * state_ + area;
			}
		}
	}
	else
	{
		if (firstSample)
		{
			state_ = InitialState;
			areaWindow_.clear();
			timeWindow_.clear();

			if (rActive)
			{
				state_ = area;
				areaWindow_.push_back(area);
				timeWindow_.push_back(t);
			}
		}
		else
		{
			if (rActive)
			{
				state_ = area;
				areaWindow_.clear();
				timeWindow_.clear();
				areaWindow_.push_back(area);
				timeWindow_.push_back(t);
			}
			else
			{
				state_ += area;
				areaWindow_.push_back(area);
				timeWindow_.push_back(t);

				if (UseIntegrationWindow == WIN_DEFINED_IN_TIME)
				{
					while (!areaWindow_.empty() &&
						(t - timeWindow_.front()) > IntegrationTime)
					{
						state_ -= areaWindow_.front();
						areaWindow_.pop_front();
						timeWindow_.pop_front();
					}
				}
				else if (UseIntegrationWindow == WIN_DEFINED_IN_SAMPLES)
				{
					while ((int)areaWindow_.size() > IntegrationSamples)
					{
						state_ -= areaWindow_.front();
						areaWindow_.pop_front();
						timeWindow_.pop_front();
					}
				}
			}
		}
	}

	prevInput_ = x;
	lastTime_ = t;
	haveLastTime_ = true;

	applyLimits();
	output[0] = state_;

	return true;
}
