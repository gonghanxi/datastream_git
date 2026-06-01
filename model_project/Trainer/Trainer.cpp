#include "Trainer.h"

namespace SystemVueModelBuilder
{

#ifndef SV_CODE_GEN
	DEFINE_MODEL_INTERFACE(Trainer)
	{
		SET_MODEL_DESCRIPTION("Initial Sample Trainer");
		SET_MODEL_SYMBOL("SYM_Trainer");
		SET_MODEL_CATEGORY("Routers/Resamplers");

		{
			DFPort p = ADD_MODEL_INPUT(train);
			p.SetName("train");
			p.SetDescription("training sequence input");
		}
		{
			DFPort p = ADD_MODEL_INPUT(decision);
			p.SetName("decision");
			p.SetDescription("test sequence input");
		}

		{
			DFPort p = ADD_MODEL_OUTPUT(output);
			p.SetName("output");
			p.SetDescription("current sequence output");
		}

		{
			DFParam param = ADD_MODEL_PARAM(TrainLength);
			param.SetName("TrainLength");
			param.SetUnit(Units::NONE);
			param.SetDefaultValue("100");
			param.SetDescription("Number of training samples to use");
		}

		return true;
	}
#endif 

	Trainer::Trainer()
		: train()
		, decision()
		, output()
		, TrainLength(100)
		, count(0)
	{
	}

	bool Trainer::Setup()
	{
		train.SetRate(1U);
		decision.SetRate(1U);
		output.SetRate(1U);

		return true;
	}

	bool Trainer::Initialize()
	{
		if (TrainLength < 0)
		{
			POST_ERROR("TrainLength must be > 0");
			return false;
		}

		count = 0;
		return true;
	}

	bool Trainer::Run()
	{
		if (count < TrainLength)
		{
			train.Copy(0U, &output, 0U, 1U);
			++count;
		}
		else
		{
			decision.Copy(0U, &output, 0U, 1U);
		}

		return true;
	}

} 
