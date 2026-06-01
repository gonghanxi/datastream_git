#include "LookUpTable.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(LookUpTable)
{
	SET_MODEL_DESCRIPTION("Mapper using Indexed Lookup Table");
	SET_MODEL_SYMBOL("SYM_LookUpTable");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("Integer index input");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("Output from the Look Up Table");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Values);
		param.SetName("Values");
		param.SetDescription("Table of output values");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[-1; 1]");
	}

	return true;
}
#endif  

LookUpTable::LookUpTable()
	: Values(2, 1),
	numDims_(2),
	firstDim_(2),
	numElements_(2)
{
	Values.Zero();
}

bool LookUpTable::Setup()
{
	numDims_ = Values.NumDimensions();
	firstDim_ = Values.Size(0);
	numElements_ = Values.NumElements();

	input.SetRate(1U);
	output.SetRate(1U);

	return true;
}

bool LookUpTable::Run()
{
	const int idx = input[0];

	if (idx < 0 || static_cast<std::size_t>(idx) >= numElements_) {
		POST_ERROR("The \"input\" value (i.e. index for the Look Up Table) "
			"must be >= 0 and < the number of data in the \"Values\" table.");
		return false;
	}

	output[0] = Values(static_cast<std::size_t>(idx));

	return true;
}
