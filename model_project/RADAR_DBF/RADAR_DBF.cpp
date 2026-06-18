#include "RADAR_DBF.h"

#include <algorithm>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_DBF)
{
	SET_MODEL_DESCRIPTION("Digital Beamforming");
	SET_MODEL_CATEGORY("Signal Processing");

	// ============================================================
	// 端口定义
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("Input signal");
	}

	{
		auto p = ADD_MODEL_INPUT(weight);
		p.SetName("weight");
		p.SetDescription("Weight for digital beamforming");
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("Output signal after beamforming");
	}

	// 帮助文档中该模型没有 Model Parameters。
	return true;
}
#endif


// ============================================================
// 构造函数
// ============================================================
RADAR_DBF::RADAR_DBF()
	: input()
	, weight()
	, output()
	, inputBusSize_(0)
	, weightBusSize_(0)
	, activeBusSize_(0)
{
}


// ============================================================
// Setup 辅助函数：检查 bus 宽度并准备内部尺寸
// ============================================================
bool RADAR_DBF::validateAndPrepare_()
{
	inputBusSize_ = static_cast<int>(input.GetSize());
	weightBusSize_ = static_cast<int>(weight.GetSize());

	if (inputBusSize_ < 0)
		inputBusSize_ = 0;

	if (weightBusSize_ < 0)
		weightBusSize_ = 0;

	if (inputBusSize_ == 0)
	{
		POST_ERROR("RADAR_DBF input bus size must be greater than 0.");
		return false;
	}

	if (weightBusSize_ == 0)
	{
		POST_ERROR("RADAR_DBF weight bus size must be greater than 0.");
		return false;
	}

	// 帮助文档说明 weight 是每个 input signal 对应的权值，因此两路 multiple complex
	// 的 bus 宽度应一致。若后续黑盒发现内置按较小宽度计算，可将这里改为 min 方式。
	if (inputBusSize_ != weightBusSize_)
	{
		POST_ERROR("RADAR_DBF input and weight bus sizes must be equal.");
		return false;
	}

	activeBusSize_ = inputBusSize_;
	return true;
}


// ============================================================
// 设置 input/weight 每个 lane 的速率
// ============================================================
void RADAR_DBF::applyInputRates_()
{
	const size_t inSize = input.GetSize();
	for (size_t k = 0; k < inSize; ++k)
	{
		input[k].SetRate(1u);
	}

	const size_t wSize = weight.GetSize();
	for (size_t k = 0; k < wSize; ++k)
	{
		weight[k].SetRate(1u);
	}
}


// ============================================================
// Setup：对齐帮助文档 rate
// ============================================================
bool RADAR_DBF::Setup()
{
	if (!validateAndPrepare_())
		return false;

	// 帮助文档：
	//   Input  rate = 1
	//   Weight rate = 1
	//   Output rate = 1
	applyInputRates_();
	output.SetRate(1u);

	return true;
}


// ============================================================
// Run：逐通道复数乘加完成数字波束形成
// ============================================================
bool RADAR_DBF::Run()
{
	// 每次 firing 重新获取 bus 宽度，避免 schematic 中 bus 宽度改变后内部尺寸滞后。
	inputBusSize_ = static_cast<int>(input.GetSize());
	weightBusSize_ = static_cast<int>(weight.GetSize());

	if (inputBusSize_ <= 0 || weightBusSize_ <= 0)
	{
		output[0] = Cx(0.0, 0.0);
		return true;
	}

	if (inputBusSize_ != weightBusSize_)
	{
		POST_ERROR("RADAR_DBF input and weight bus sizes must be equal.");
		return false;
	}

	activeBusSize_ = inputBusSize_;

	Cx acc(0.0, 0.0);

	for (int k = 0; k < activeBusSize_; ++k)
	{
		// 内置最可能实现：output = sum(input[k] * weight[k])
		// 不取共轭、不做归一化。权值的生成和共轭约定由上游 RADAR_ADBF 等模型负责。
		acc += getInputCx_(k) * getWeightCx_(k);
	}

	output[0] = acc;
	return true;
}


// ============================================================
// 读取 input bus 第 index 路当前 token
// ============================================================
RADAR_DBF::Cx RADAR_DBF::getInputCx_(int index)
{
	if (index < 0)
		return Cx(0.0, 0.0);

	const size_t busSize = input.GetSize();
	if (static_cast<size_t>(index) >= busSize)
		return Cx(0.0, 0.0);

	return input[static_cast<size_t>(index)][0];
}


// ============================================================
// 读取 weight bus 第 index 路当前 token
// 注意：同 getInputCx_，这里也保持非 const 以匹配 SystemVue 2020 的 buffer 访问接口。
// ============================================================
RADAR_DBF::Cx RADAR_DBF::getWeightCx_(int index)
{
	if (index < 0)
		return Cx(0.0, 0.0);

	const size_t busSize = weight.GetSize();
	if (static_cast<size_t>(index) >= busSize)
		return Cx(0.0, 0.0);

	return weight[static_cast<size_t>(index)][0];
}
