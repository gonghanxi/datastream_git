#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <vector>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API IntToBits : public SystemVueModelBuilder::DFModel {
public:

	enum BitOrderEnum {
		LSB_first = 0,
		MSB_first = 1
	};

	DECLARE_MODEL_INTERFACE(IntToBits);

	IntToBits();
	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<int> input;
	SystemVueModelBuilder::CircularBuffer<int> output;

	int          NumBits;
	BitOrderEnum BitOrder;

private:
	std::vector<int>  bitBuffer;
	std::size_t       bitIndex;
};
