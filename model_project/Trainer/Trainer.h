#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{

    class SYSTEMVUEMODELBUILDER_API Trainer : public SystemVueModelBuilder::DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(Trainer);

		Trainer();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

		SystemVueModelBuilder::CircularBuffer<double> train;    
		SystemVueModelBuilder::CircularBuffer<double> decision; 
		SystemVueModelBuilder::CircularBuffer<double> output;   

		int TrainLength;

	protected:
		int count;
	};

} 
