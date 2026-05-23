#include "IntegratorCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(IntegratorCx)
{
	SET_MODEL_DESCRIPTION("Complex Integrator with Reset");
	SET_MODEL_SYMBOL("SYM_Integrator");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		auto p = ADD_MODEL_INPUT(reset);
		p.SetDescription("reset");
		p.SetOptional(true);   
	}
	{
		auto p = ADD_MODEL_INPUT(data);
		p.SetDescription("input (complex)");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output (complex)");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(IntegrationMethod, IntegrationMethodEnum);
		p.AddEnumeration("Rectangle", IntegratorCx::RECTANGLE);
		p.AddEnumeration("Trapezoidal", IntegratorCx::TRAPEZOIDAL);
		p.SetDefaultValue("Rectangle");
		p.SetDescription("Integration method");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(LimitOutput, LimitOutputEnum);
		p.AddEnumeration("No", IntegratorCx::LIMIT_NO);
		p.AddEnumeration("Saturate", IntegratorCx::LIMIT_SATURATE);
		p.AddEnumeration("Wrap", IntegratorCx::LIMIT_WRAP);
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
		p.SetDescription("Initial integrator state (complex)");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(UseIntegrationWindow, WindowEnum);
		p.AddEnumeration("No", IntegratorCx::WIN_NO);
		p.AddEnumeration("DefinedInTime", IntegratorCx::WIN_DEFINED_IN_TIME);
		p.AddEnumeration("DefinedInSamples", IntegratorCx::WIN_DEFINED_IN_SAMPLES);
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

IntegratorCx::IntegratorCx()
	: IntegrationMethod(RECTANGLE),
	LimitOutput(LIMIT_NO),
	Top(0.0),
	Bottom(0.0),
	InitialState(0.0, 0.0),
	UseIntegrationWindow(WIN_NO),
	FeedbackGain(1.0),
	IntegrationTime(100e-6),
	IntegrationSamples(100),
	Ts_(0.0),
	lastTime_(0.0),
	haveLastTime_(false),
	state_(0.0, 0.0),
	prevInput_(0.0, 0.0),
	resetConnected_(false)
{
}

bool IntegratorCx::Setup()
{
	resetConnected_ = reset.IsConnected();

	Ts_ = 0.0;
	haveLastTime_ = false;
	lastTime_ = 0.0;

	state_ = InitialState;          
	prevInput_ = Complex(0.0, 0.0);

	areaWindow_.clear();
	timeWindow_.clear();

	if (data.GetSampleRate() > 0.0)
		output.SetSampleRate(data.GetSampleRate());

	return true;
}

IntegratorCx::Complex
IntegratorCx::computeArea(const Complex& xPrev, const Complex& xCurr, double Ts)
{
	if (Ts <= 0.0)
		return Complex(0.0, 0.0);

	if (IntegrationMethod == RECTANGLE)
	{
		return Complex(Ts, 0.0) * xCurr;
	}
	else 
	{
		return Complex(0.5 * Ts, 0.0) * (xPrev + xCurr);
	}
}

void IntegratorCx::applyLimits()
{
	if (LimitOutput == LIMIT_NO)
		return;

	double top = Top;
	double bottom = Bottom;
	if (top < bottom)
		std::swap(top, bottom);

	if (LimitOutput == LIMIT_SATURATE)
	{
		auto sat = [top, bottom](double v) {
			if (v > top)    return top;
			if (v < bottom) return bottom;
			return v;
		};
		state_ = Complex(sat(state_.real()), sat(state_.imag()));
	}
	else if (LimitOutput == LIMIT_WRAP)
	{
		const double span = top - bottom;
		if (span <= 0.0)
			return;

		auto wrap = [top, bottom, span](double v) {
			double x = v - bottom;
			x = std::fmod(x, span);
			if (x < 0.0) x += span;
			return bottom + x;
		};
		state_ = Complex(wrap(state_.real()), wrap(state_.imag()));
	}
}

bool IntegratorCx::Run()
{
	const double  t = data.GetTime(0, GetCount());
	const Complex x = data[0];
	const bool    rActive = resetConnected_ && (reset[0] != 0);
	const bool    firstSample = !haveLastTime_;

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

	Complex area;
	if (IntegrationMethod == RECTANGLE)
	{
		area = Complex(Ts, 0.0) * x;
	}
	else
	{
		const Complex xPrev = firstSample ? x : prevInput_;
		area = Complex(0.5 * Ts, 0.0) * (xPrev + x);
	}

	const Complex init = InitialState;

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
				state_ = FeedbackGain * init + area;
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
			state_ = init;
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
