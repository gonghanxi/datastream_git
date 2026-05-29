#include "EnvFcChange_M.h"

#include <cmath>
#include <complex>
#include <algorithm>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(EnvFcChange_M)
{
	SET_MODEL_DESCRIPTION("Envelope Signal Characterization Frequency Converter (Matrix)");
	SET_MODEL_SYMBOL("SYM_EnvFcChange");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Beamforming");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("input signal (envelope matrix)");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output signal (envelope matrix)");
	}

	{
		DFParam p = ADD_MODEL_PARAM(OutputFc);
		p.SetUnit(Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("New characterization frequency");
	}

	{
		DFParam p = ADD_MODEL_PARAM(Bandwidth);
		p.SetUnit(Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("Bandwidth of bandpass filter centered at OutputFc (used when input fc=0)");
	}

	return true;
}
#endif

EnvFcChange_M::EnvFcChange_M()
	: OutputFc(0.0)
	, Bandwidth(0.0)
	, fc_in_(0.0)
	, fc_out_(0.0)
	, lpfInitialized_(false)
	, lpfNumElements_(0)
{
}

ERESULT EnvFcChange_M::PropagateCharacterizationFrequency()
{
	fc_in_ = input.GetCharacterizationFrequency();

	// 内置 EnvFcChange/EnvFcChange_M 中，OutputFc = 0 表示新的表征频率就是 0 Hz，
	// 不是保持输入 Fc 不变。
	fc_out_ = OutputFc;

	if (!std::isfinite(fc_in_) || fc_in_ < 0.0)
		fc_in_ = 0.0;

	if (!std::isfinite(fc_out_) || fc_out_ < 0.0)
		fc_out_ = 0.0;

	output.SetCharacterizationFrequency(fc_out_);

	return true;
}

bool EnvFcChange_M::Setup()
{
	(void)PropagateCharacterizationFrequency();

	output.SetRate(1U);

	lpfState_.clear();
	lpfInitialized_ = false;
	lpfNumElements_ = 0;

	return true;
}

void EnvFcChange_M::resetLpfStateIfNeeded(size_t numElements)
{
	if (!lpfInitialized_ || lpfNumElements_ != numElements)
	{
		lpfState_.assign(numElements, std::complex<double>(0.0, 0.0));
		lpfNumElements_ = numElements;
		lpfInitialized_ = true;
	}
}

double EnvFcChange_M::getEffectiveBandwidth() const
{
	// 帮助文档说明：
	// input fc = 0 时使用 Bandwidth；
	// 如果 Bandwidth = 0，则认为 Bandwidth 等于 f2，也就是 OutputFc。
	double bw = Bandwidth;

	if (!std::isfinite(bw) || bw < 0.0)
		bw = 0.0;

	if (bw == 0.0)
		bw = fc_out_;

	if (!std::isfinite(bw) || bw < 0.0)
		bw = 0.0;

	return bw;
}

double EnvFcChange_M::getInputTimeStep() const
{
	double ts = input.GetTimeStep();

	if (!std::isfinite(ts) || ts <= 0.0)
	{
		const double fs = input.GetSampleRate();
		if (std::isfinite(fs) && fs > 0.0)
			ts = 1.0 / fs;
	}

	if (!std::isfinite(ts) || ts <= 0.0)
		ts = 1.0;

	return ts;
}

bool EnvFcChange_M::Run()
{
	// 使用输入样本真实时间，避免前级 StartTime / Delay 改变时相位错位。
	const double t = input.GetTime(0, GetCount());

	const EnvelopeMatrix& xin = input[0];

	EnvelopeMatrix yout;
	yout.Resize(xin.NumRows(), xin.NumColumns());

	const double fc_in = fc_in_;
	const double fc_out = fc_out_;

	const size_t numElements = xin.NumElements();

	// ============================================================
	// 特殊分支：
	// input fc = 0 且 OutputFc > 0
	//
	// 根据内置帮助文档：
	// When f1 = 0, the input signal is treated as a bandpass signal.
	// I/Q are extracted by multiplying real input by cos/sin and
	// low-pass filtering the products.
	//
	// 注意：
	// 帮助文档没有公开内置 LPF 的阶数/群时延/窗函数。
	// 这里用一阶 IIR 低通近似该分支。
	// ============================================================
	if (fc_in == 0.0 && fc_out > 0.0)
	{
		resetLpfStateIfNeeded(numElements);

		const double bw = getEffectiveBandwidth();
		const double ts = getInputTimeStep();

		// 一阶低通：
		// y[n] = y[n-1] + alpha * (x[n] - y[n-1])
		// alpha = 1 - exp(-2*pi*bw*Ts)
		double alpha = 1.0;

		if (bw > 0.0 && ts > 0.0)
		{
			alpha = 1.0 - std::exp(-2.0 * kPI * bw * ts);
			if (alpha < 0.0) alpha = 0.0;
			if (alpha > 1.0) alpha = 1.0;
		}

		const double phase = 2.0 * kPI * fc_out * t;
		const double c = std::cos(phase);
		const double s = std::sin(phase);

		for (size_t i = 0; i < numElements; ++i)
		{
			const EnvelopeSignal& ein = xin(i);

			// fc_in = 0 时，把输入按实带通信号处理。
			// 这里取 real 部分作为 x(t)。
			const std::complex<double> raw = ein.ConvertToNewFc(0.0, 0.0, t);
			const double x = raw.real();

			// 为了与 SystemVue envelope 约定一致：
			// real{(I+jQ) * exp(j*w*t)} = I*cos(wt) - Q*sin(wt)
			//
			// 所以由实带通信号 x(t) 提取 envelope 时：
			// I ≈ LPF( 2*x*cos(wt) )
			// Q ≈ LPF(-2*x*sin(wt) )
			std::complex<double> mixed(
				2.0 * x * c,
				-2.0 * x * s
			);

			lpfState_[i] = lpfState_[i] + alpha * (mixed - lpfState_[i]);

			CopyToEnvelopeSignal(lpfState_[i], yout(i));
		}

		output[0] = yout;
		return true;
	}

	// ============================================================
	// 常规分支：
	// input fc > 0，或者 input/output fc 相同。
	//
	// 根据帮助文档：
	// v2(t) = v1(t) * exp(j*2*pi*(f1-f2)*t), f1 > 0
	// 如果 f2 = 0，则 v2(t) 的虚部置零。
	// ============================================================
	for (size_t i = 0; i < numElements; ++i)
	{
		const EnvelopeSignal& ein = xin(i);

		if (fc_in != fc_out || fc_out == 0.0)
		{
			std::complex<double> cx_new =
				ein.ConvertToNewFc(fc_in, fc_out, t);

			// 内置帮助文档说明：
			// If f2 = 0, then the imaginary part of v2(t) is set to zero.
			if (fc_out == 0.0)
			{
				cx_new = std::complex<double>(cx_new.real(), 0.0);
			}

			CopyToEnvelopeSignal(cx_new, yout(i));
		}
		else
		{
			// Fc 相同且不为 0 时，直接复制 EnvelopeSignal。
			yout(i) = ein;
		}
	}

	output[0] = yout;
	return true;
}