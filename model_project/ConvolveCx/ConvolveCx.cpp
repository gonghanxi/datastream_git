#include "ConvolveCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(ConvolveCx)
{
	using SystemVueModelBuilder::DFParam;

	SET_MODEL_DESCRIPTION("Convoluton Functon");
	SET_MODEL_SYMBOL("SYM_Convolve");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		auto p = ADD_MODEL_INPUT(inA);
		p.SetDescription("Input A");
	}
	{
		auto p = ADD_MODEL_INPUT(inB);
		p.SetDescription("Input B");
	}
	{
		auto p = ADD_MODEL_OUTPUT(out);
		p.SetDescription("Output");
	}

	{
		DFParam p = ADD_MODEL_PARAM(TruncationDepth);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("256");
		p.SetDescription("Maximum number of terms in convolution sum");
	}

	return true;
}
#endif

ConvolveCx::ConvolveCx()
	: TruncationDepth(256),
	depth_(0),
	sampleCount_(0)
{}

bool ConvolveCx::Setup()
{
	if (TruncationDepth <= 0) {
		POST_ERROR("ConvolveCx: TruncationDepth must be > 0.");
		return false;
	}

	depth_ = static_cast<std::size_t>(TruncationDepth);
	sampleCount_ = 0;

	histA_.assign(depth_, std::complex<double>(0.0, 0.0));
	histB_.assign(depth_, std::complex<double>(0.0, 0.0));

	return true;
}

bool ConvolveCx::Run()
{
	// 1) 只对 inB 做历史移位：
	//    histB_[0] = inB[n]
	//    histB_[1] = inB[n-1]
	//    ...
	for (std::size_t k = depth_ - 1; k > 0; --k) {
		histB_[k] = histB_[k - 1];
	}
	histB_[0] = inB[0];

	// 2) inA 按“样点序号”顺序写入一次，写满 depth_ 后保持不变
	//    histA_[0] = inA[0]
	//    histA_[1] = inA[1]
	//    ...
	if (sampleCount_ < depth_) {
		histA_[sampleCount_] = inA[0];
	}

	// 3) 按内置帮助文档公式计算：
	//    out[n] = sum_{k=0}^{T-1} inA[k] * conj(inB[n-k])
	std::complex<double> acc(0.0, 0.0);

	const std::size_t terms =
		(sampleCount_ + 1 < depth_) ? (sampleCount_ + 1) : depth_;

	for (std::size_t k = 0; k < terms; ++k) {
		acc += histA_[k] * std::conj(histB_[k]);
	}

	out[0] = acc;

	++sampleCount_;
	return true;
}