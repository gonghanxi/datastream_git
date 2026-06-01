#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <cstddef>

namespace SystemVueModelBuilder
{

	class ChopVarOffset : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(ChopVarOffset);

		ChopVarOffset();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

		SystemVueModelBuilder::CircularBuffer<double> input;       
		SystemVueModelBuilder::CircularBuffer<int>    offsetCntrl; 
		SystemVueModelBuilder::CircularBuffer<double> output;      

		int nRead;   
		int nWrite; 

	protected:
		int Offset;

		std::size_t iReadFrom;
		std::size_t iReadNum;      
		std::size_t iReadBufSize;

		std::size_t iWriteTo;
		std::size_t iWriteNum;
		std::size_t iWriteBufSize;

		std::size_t iZeroPadFrom;
		std::size_t iZeroPadNum;

		void ComputeRange();
	};

} 
