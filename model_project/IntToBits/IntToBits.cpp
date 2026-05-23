#include "IntToBits.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(IntToBits)
{
	SET_MODEL_DESCRIPTION("Integer to Bits Converter");
	SET_MODEL_SYMBOL("SYM_IntToBits");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);


	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(NumBits);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("4");
		p.SetDescription("Number of output bits per input sample");
	}


	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(BitOrder, BitOrderEnum);
		enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);

		enumParam.AddEnumeration("LSB first", LSB_first);
		enumParam.AddEnumeration("MSB first", MSB_first);

		enumParam.SetDefaultValue("MSB first");

		enumParam.SetDescription("Bit order");
	}

	return true;
}
#endif

IntToBits::IntToBits()
	: NumBits(4), bitIndex(0)
{
	bitBuffer.clear();
}

bool IntToBits::Setup()
{
	if (NumBits < 1 || NumBits > 32) {
		POST_ERROR("NumBits must be between 1 and 32.");
		return false;
	}

	bitBuffer.assign(static_cast<std::size_t>(NumBits), 0);
	bitIndex = 0;

	// 与现有逻辑保持一致：一次 Run 写 NumBits 个样本
	output.SetRate(static_cast<unsigned>(NumBits));
	return true;
}

bool IntToBits::Run()
{
	const int N = NumBits;

	const unsigned int uval = static_cast<unsigned int>(input[0]);

	// 读取当前枚举值（0:LSB_first, 1:MSB_first）
	const int order = (BitOrder == LSB_first) ? 0 : 1;

	for (int k = 0; k < N; ++k) {
		const int b = ((uval >> k) & 0x1u) ? 1 : 0;
		if (order == 0) {
			bitBuffer[static_cast<std::size_t>(k)] = b;           // LSB→MSB
		}
		else {
			bitBuffer[static_cast<std::size_t>(N - 1 - k)] = b;   // MSB→LSB
		}
	}

	for (int i = 0; i < N; ++i) {
		output[static_cast<unsigned>(i)] = bitBuffer[static_cast<std::size_t>(i)];
	}

	bitIndex = 0; 
	return true;
}
