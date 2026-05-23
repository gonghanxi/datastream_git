#include "UpSampleEnv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(UpSampleEnv)
{
	SET_MODEL_DESCRIPTION("Up Sampler for Envelope Signal");
	SET_MODEL_SYMBOL("SYM_UpSampleEnv");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	// Add port
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("input envelope signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("output envelope signal");
	}

	// 参数：Factor
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Factor);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetName("Factor");
		p.SetDefaultValue("5");
		p.SetDescription("Upsampling ratio");
	}

	// 参数：Mode（枚举）
	{
		SystemVueModelBuilder::DFParam e = ADD_MODEL_ENUM_PARAM(Mode, ModeEnum);
		e.SetUnit(SystemVueModelBuilder::Units::NONE);
		e.AddEnumeration("Insert zeros", Insertzeros);
		e.AddEnumeration("Hold sample", Holdsample);
		//e.AddEnumeration("Polyphase filter", Polyphasefilter);
		e.AddEnumeration("Linear", Linear);
		e.SetDefaultValue("Hold sample");
		e.SetDescription("Upsampling interpolation type");
		e.SetSchematicDisplay(false);
	}

	// 参数：Phase
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Phase);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Upsampling insertion phase for the output non-zero sample");
		p.SetHideCondition("Mode ~= 0 || Factor == 1");
		p.SetSchematicDisplay(false);	}

	// 参数：ExcessBW
	//{
	//	SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ExcessBW);
	//	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	p.SetDefaultValue("0.5");
	//	p.SetDescription("Excess bandwidth of raised cosine interpolation filter");
	//	p.SetHideCondition("Mode ~= 2");
	//	p.SetSchematicDisplay(false);
	//}
	return true;
}
#endif

UpSampleEnv::UpSampleEnv()
{
	m_bIsInRun = false;
}

ERESULT UpSampleEnv::PropagateCharacterizationFrequency()
{
	bool bStatus = true;
	FcOut = input.GetCharacterizationFrequency();

	output.SetCharacterizationFrequency(FcOut);

	return bStatus;
}

bool UpSampleEnv::Setup()
{
	if (Factor < 1)
	{
		POST_ERROR("Factor must be >= 1");
		return false;
	}
	else
		output.SetRate(Factor);
	return true;
}

bool UpSampleEnv::Initialize()
{
	if (Mode == ModeEnum::Insertzeros)
	{
		if (m_bIsInRun == false)
		{
			if ((Phase < 0) || (Factor > 0 && Phase >= Factor))
			{
				POST_ERROR("Phase must be > 0 and Phase must be < Factor");
				return false;
			}
		}
		else
		{
			if (Phase < 0)
				Phase = 0;
			else if (Factor > 0 && Phase >= Factor)
				Phase = Factor;
		}
		return true;
	}

	if (Mode == ModeEnum::Polyphasefilter)
	{
		if (ExcessBW < 0 || ExcessBW > 1)
		{
			POST_ERROR("ExcessBW must be >= 0 and <= 1");
			return false;
		}
		else
		    return true;
	}

	m_bIsInRun = true;
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool UpSampleEnv::Run()
{
	size_t inLen = input.GetSize();
	size_t outLen = inLen * Factor;
	//output.AllocateMemory(outLen);

	if (Mode == ModeEnum::Insertzeros)
	{
		// Insert zeros between samples
		for (size_t i = 0; i < inLen; ++i)
		{
			input.Copy(i, &output, i * Factor + Phase, 1);
			if (Factor > 1)
			{
				for (size_t j = 1; j < Factor; ++j)
				{
					output.Zero(i * Factor + Phase + j, 1);
				}
			}
		}
	}
	else if (Mode == ModeEnum::Holdsample)
	{
		// Hold the previous sample value
		for (size_t i = 0; i < inLen; ++i)
		{
			for (size_t j = 0; j < Factor; ++j)
			{
				input.Copy(i, &output, i * Factor + j, 1);
			}
		}
	}

	else if (Mode == ModeEnum::Linear)
	{
		// 初始化变量
		currentEnv = input[0].complex();
		size_t outLen;
		std::complex<double> prevCx;
		std::complex<double> currCx;

		// 处理第一次输入(第一个输出点实部虚部均为0，加上Factor个点，共Factor+1个点)
		if (!m_hasPrevSample)
		{
			outLen = Factor;

			//output.AllocateMemory(outLen); // 确保输出缓冲区长度正确

			prevCx = std::complex<double>(0.0, 0.0);

			currCx = currentEnv; // 当前样本的复数包络
			
			m_hasPrevSample = true;
		}
		else  // 后续输入从上一个输入点插值到本输入点
		{
			outLen = Factor;

			//output.AllocateMemory(outLen); // 确保输出缓冲区长度正确

			prevCx = m_prevEnv; // 前样本的复数包络

			currCx = currentEnv; // 当前样本的复数包络
		}

		// 线性插值：在[prevCx, currCx]之间插入Factor个点（包括prevCx）
		for (size_t j = 0; j < outLen; j++)
		{
			double t = static_cast<double>(j) / Factor;

			std::complex<double> interpCx = prevCx + t * (currCx - prevCx);

			interpEnv = input;
			interpEnv.SetCharacterizationFrequency(FcOut);
			
		    interpEnv[0] = interpCx;

			interpEnv.Copy(0, &output, j, 1);
		}

		// 缓存当前样本（供下一次插值使用）
		m_prevEnv = currentEnv;
	}

	//else if (Mode == ModeEnum::Polyphasefilter)
	//{
	//	// Design polyphase filter
	//	int numTaps = (1 + 20 * Factor); // Filter taps number
	//	std::vector<float> filterCoeffs = DesignPolyphaseFilter(Factor, ExcessBW, numTaps);

	//	// Decompose into sub-filters
	//	std::vector<std::vector<float>> polyPhaseFilters(Factor);
	//	for (int i = 0; i < numTaps; ++i)
	//	{
	//		polyPhaseFilters[i % Factor].push_back(filterCoeffs[i]);
	//	}

	//	// Convolution calculation
	//	for (size_t i = 0; i < inLen; ++i)
	//	{
	//		for (int j = 0; j < Factor; ++j)
	//		{
	//			float sum = 0.0f;
	//			for (size_t k = 0; k < polyPhaseFilters[j].size(); ++k)
	//			{
	//				if (i - k >= 0 && i - k < inLen)
	//				{
	//					SystemVueModelBuilder::EnvelopeCircularBuffer tempValue;
	//					input.Copy(i - k, &tempValue, 0, 1);
	//					sum += tempValue * polyPhaseFilters[j][k];
	//				}
	//			}
	//			output.Copy(i * Factor + j, &sum, 0, 1);
	//		}
	//	}
	//}
	

	return true;
}

// 设计多相滤波器的辅助函数
std::vector<double> UpSampleEnv::DesignPolyphaseFilter(int factor, float excessBW, int numTaps)
{
	// 这里需要具体的滤波器设计算法，例如使用窗函数法或频率采样法
	// 示例：使用简单的低通滤波器设计
	std::vector<double> coeffs(numTaps, 0.0f);
	float fCutoff = 1.0f / factor;
	float beta = 8.0f; // Kaiser window parameter

	for (int i = 0; i < numTaps; ++i)
	{
		float n = i - (numTaps - 1) / 2.0f;
		if (n == 0)
		{
			coeffs[i] = 2 * M_PI * fCutoff;
		}
		else
		{
			coeffs[i] = sin(2 * M_PI * fCutoff * n) / (M_PI * n);
		}
		coeffs[i] *= kaiserWindow(n, beta, numTaps);
	}

	return coeffs;
}

// Kaiser窗口函数
float UpSampleEnv::kaiserWindow(float n, float beta, int numTaps)
{
	float alpha = beta * sqrt(1.0f - pow((n / ((numTaps - 1) / 2.0f)), 2));
	return besselI0(alpha) / besselI0(beta);
}

// 第一类贝塞尔函数I0
float UpSampleEnv::besselI0(float x)
{
	const int N = 10;
	float sum = 1.0f;
	float term = 1.0f;
	for (int k = 1; k <= N; ++k)
	{
		term *= x * x / (4.0f * k * k);
		sum += term;
	}
	return sum;
}