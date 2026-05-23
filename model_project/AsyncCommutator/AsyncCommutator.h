#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <sstream>

class SYSTEMVUEMODELBUILDER_API AsyncCommutator : public SystemVueModelBuilder::DFModel
{
public:
	using BufferType = SystemVueModelBuilder::CircularBuffer<double>;
	using BusType = SystemVueModelBuilder::CircularBufferBusT<BufferType>;

	AsyncCommutator();

	bool Setup() override;
	bool Run()   override;

	BusType    input;    
	BufferType output;   

	SystemVueModelBuilder::Matrix<int> BlockSizes;

	DECLARE_MODEL_INTERFACE(AsyncCommutator);
};
