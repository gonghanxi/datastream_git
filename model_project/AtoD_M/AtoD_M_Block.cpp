#include "AtoD_M_Block.h"

#include <algorithm>
#include <ctime>
#include <sstream>
#include <string>

const double AtoD_M_Block::kPi   = 3.1415926535897932384626433832795;
const double AtoD_M_Block::kTiny = 1e-30;

// ===== 字符串工具函数 =====
namespace {
std::string TrimCopy(const std::string& value)
{
	std::string s = value;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		[](unsigned char ch) { return !std::isspace(ch); }));
	s.erase(std::find_if(s.rbegin(), s.rend(),
		[](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
	return s;
}

std::string ToLowerCopy(const std::string& value)
{
	std::string s = value;
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return s;
}
} // namespace

// ===== 数组字符串解析 =====
bool AtoD_M_Block::parseArrayString(const std::string& arrayStr, std::vector<double>& outArray)
{
	outArray.clear();
	std::string str = arrayStr;
	size_t start = str.find_first_not_of(" \t\n\r");
	if (start == std::string::npos) return false;
	size_t end = str.find_last_not_of(" \t\n\r");
	str = str.substr(start, end - start + 1);
	if (str.empty() || str.front() != '[' || str.back() != ']') return false;

	std::string content = str.substr(1, str.length() - 2);
	start = content.find_first_not_of(" \t\n\r");
	if (start == std::string::npos) return true;
	end = content.find_last_not_of(" \t\n\r");
	content = content.substr(start, end - start + 1);

	std::stringstream ss(content);
	std::string item;
	while (std::getline(ss, item, ','))
	{
		start = item.find_first_not_of(" \t\n\r");
		if (start == std::string::npos) continue;
		end = item.find_last_not_of(" \t\n\r");
		item = item.substr(start, end - start + 1);
		if (!item.empty())
		{
			try {
				double value = std::stod(item);
				outArray.push_back(value);
			} catch (const std::exception& e) {
				LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
				return false;
			}
		}
	}
	return true;
}

// ===== 构造函数 =====
AtoD_M_Block::AtoD_M_Block(const std::string& name)
	: Block(name)
{
}

// ===== 默认参数 =====
void AtoD_M_Block::SetDefaultParamters()
{
	m_NBits                = 8;
	m_VRef                 = 1.0;
	m_OutputDigitalFormat  = AtoD_M::Offset_binary;
	m_DistortionModel      = AtoD_M::Jitter_INL_DNL;
	m_EnableJitter         = AtoD_M::Jitter_No;
	m_RJrms                = 0.0;
	m_PhaseNoiseVector.clear();
	m_PN_Type              = AtoD_M::Random_PN;

	m_INL  = 0.0;
	m_DNL  = 0.0;

	m_ENOB    = 7;
	m_SNR_dB  = 60.0;
	m_H2_dBc  = -400.0;
	m_H3_dBc  = -400.0;
	m_H4_dBc  = -400.0;
	m_H5_dBc  = -400.0;

	m_SINAD_dB  = 60.0;
	m_SFDR_dBc  = 70.0;
	m_FFT_Size  = AtoD_M::FFT_2_14;

	m_SNR_Model             = AtoD_M::Quantization_and_Jitter;
	m_ThermalNoise_SNR_dBFS = 63;
	m_CenterFreq            = 100.0e6;
	m_Level_dBFS            = 0.0;

	m_ConversionType    = AtoD_M::Clocked;
	m_Clock             = 0.2e6;
	m_Phase             = 0.0;

	m_DownsampleFactor   = 1;
	m_DownsamplePhase    = 0;
	m_AntiAliasingFilter = AtoD_M::AA_OFF;
	m_ExcessBW           = 0.5;
}

// ===== 参数生效：执行 clamp + 建表 =====
void AtoD_M_Block::SetParameters()
{
	if (!m_atod_m) return;

	// 同步参数到原算法对象（用于端口绑定与 GetCount）
	m_atod_m->NBits                = m_NBits;
	m_atod_m->VRef                 = m_VRef;
	m_atod_m->OutputDigitalFormat  = m_OutputDigitalFormat;
	m_atod_m->DistortionModel      = m_DistortionModel;
	m_atod_m->EnableJitter         = m_EnableJitter;
	m_atod_m->RJrms                = m_RJrms;
	m_atod_m->PhaseNoiseData       = m_PhaseNoiseVector.empty() ? nullptr : m_PhaseNoiseVector.data();
	m_atod_m->PhaseNoiseDataSize   = static_cast<int>(m_PhaseNoiseVector.size());
	m_atod_m->PN_Type              = m_PN_Type;
	m_atod_m->INL                   = m_INL;
	m_atod_m->DNL                   = m_DNL;
	m_atod_m->ENOB                  = m_ENOB;
	m_atod_m->SNR_dB                = m_SNR_dB;
	m_atod_m->H2_dBc                = m_H2_dBc;
	m_atod_m->H3_dBc                = m_H3_dBc;
	m_atod_m->H4_dBc                = m_H4_dBc;
	m_atod_m->H5_dBc                = m_H5_dBc;
	m_atod_m->SINAD_dB              = m_SINAD_dB;
	m_atod_m->SFDR_dBc              = m_SFDR_dBc;
	m_atod_m->FFT_Size              = m_FFT_Size;
	m_atod_m->SNR_Model             = m_SNR_Model;
	m_atod_m->ThermalNoise_SNR_dBFS = m_ThermalNoise_SNR_dBFS;
	m_atod_m->CenterFreq            = m_CenterFreq;
	m_atod_m->Level_dBFS            = m_Level_dBFS;
	m_atod_m->ConversionType        = m_ConversionType;
	m_atod_m->Clock                 = m_Clock;
	m_atod_m->Phase                 = m_Phase;
	m_atod_m->DownsampleFactor      = m_DownsampleFactor;
	m_atod_m->DownsamplePhase       = m_DownsamplePhase;
	m_atod_m->AntiAliasingFilter    = m_AntiAliasingFilter;
	m_atod_m->ExcessBW              = m_ExcessBW;
}

// ===== Initialize =====
bool AtoD_M_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);
	m_atod_m = std::make_unique<AtoD_M>();

	SetDefaultParamters();

	m_simulator_param = getSimu();

	try { m_NBits               = std::stoi(getParameter("NBits").Value);               } catch (...) {}
	try { m_VRef                 = std::stod(getParameter("VRef").Value);                } catch (...) {}
	try { m_OutputDigitalFormat  = ConvertStringToOutputDigitalFormatEnum(getParameter("OutputDigitalFormat").Value); } catch (...) {}
	try { m_DistortionModel      = ConvertStringToDistortionModelEnum(getParameter("DistortionModel").Value); } catch (...) {}
	try { m_EnableJitter         = ConvertStringToEnableJitterEnum(getParameter("EnableJitter").Value); } catch (...) {}
	try { m_RJrms                = std::stod(getParameter("RJrms").Value);               } catch (...) {}
	try {
		std::string PrimString = getParameter("PhaseNoiseData").Value;
		parseArrayString(PrimString, m_PhaseNoiseVector);
	} catch (...) {}
	try { m_PN_Type              = ConvertStringToPN_TypeEnum(getParameter("PN_Type").Value); } catch (...) {}

	try { m_INL  = std::stod(getParameter("INL").Value);  } catch (...) {}
	try { m_DNL  = std::stod(getParameter("DNL").Value);  } catch (...) {}

	try { m_ENOB    = std::stoi(getParameter("ENOB").Value);    } catch (...) {}
	try { m_SNR_dB  = std::stod(getParameter("SNR_dB").Value);  } catch (...) {}
	try { m_H2_dBc  = std::stod(getParameter("H2_dBc").Value);  } catch (...) {}
	try { m_H3_dBc  = std::stod(getParameter("H3_dBc").Value);  } catch (...) {}
	try { m_H4_dBc  = std::stod(getParameter("H4_dBc").Value);  } catch (...) {}
	try { m_H5_dBc  = std::stod(getParameter("H5_dBc").Value);  } catch (...) {}

	try { m_SINAD_dB = std::stod(getParameter("SINAD_dB").Value); } catch (...) {}
	try { m_SFDR_dBc = std::stod(getParameter("SFDR_dBc").Value); } catch (...) {}
	try { m_FFT_Size = ConvertStringToFFT_SizeEnum(getParameter("FFT_Size").Value); } catch (...) {}

	try { m_SNR_Model             = ConvertStringToSNR_ModelEnum(getParameter("SNR_Model").Value); } catch (...) {}
	try { m_ThermalNoise_SNR_dBFS = std::stoi(getParameter("ThermalNoise_SNR_dBFS").Value); } catch (...) {}
	try { m_CenterFreq  = std::stod(getParameter("CenterFreq").Value);  } catch (...) {}
	try { m_Level_dBFS  = std::stod(getParameter("Level_dBFS").Value);  } catch (...) {}

	try { m_ConversionType    = ConvertStringToConversionTypeEnum(getParameter("ConversionType").Value); } catch (...) {}
	try { m_Clock   = std::stod(getParameter("Clock").Value);   } catch (...) {}
	try { m_Phase   = std::stod(getParameter("Phase").Value);   } catch (...) {}

	try { m_DownsampleFactor   = std::stoi(getParameter("DownsampleFactor").Value);   } catch (...) {}
	try { m_DownsamplePhase    = std::stoi(getParameter("DownsamplePhase").Value);    } catch (...) {}
	try { m_AntiAliasingFilter = ConvertStringToAntiAliasingFilterEnum(getParameter("AntiAliasingFilter").Value); } catch (...) {}
	try { m_ExcessBW = std::stod(getParameter("ExcessBW").Value); } catch (...) {}

	SetParameters();

	m_sampleIndex = 0ULL;
	m_rngState    = static_cast<unsigned int>(std::time(nullptr)) ^ 0x9E3779B9U;
	reset_states_();

	// 端口注册 — 使用 m_atod_m 的 CircularBuffer
	int system_rate = 1;
	if (m_ConversionType == AtoD_M::Downsampled)
		system_rate = m_DownsampleFactor;

	m_atod_m->A_in.SetStartTime(m_simulator_param.startTime);
	m_atod_m->A_out.SetStartTime(m_simulator_param.startTime);

    AddInputPort("A_in",  m_atod_m->A_in,  system_rate, Block::DataType::MATRIX_ENVELOPE);
    AddOutputPort("A_out", m_atod_m->A_out, 1,           Block::DataType::MATRIX_ENVELOPE);
	AddOutputPort("D_I",   m_atod_m->D_I,   1,           Block::DataType::MATRIX_INT);
	AddOutputPort("D_Q",   m_atod_m->D_Q,   1,           Block::DataType::MATRIX_INT);

	return true;
}

// ===== Setup =====
bool AtoD_M_Block::Setup()
{
	Block::Setup();

	clamp_params_();
	build_transfer_table_();

	m_sampleIndex = 0ULL;
	reset_states_();
	m_rngState = static_cast<unsigned int>(std::time(nullptr)) ^ 0x9E3779B9U;

	m_inputBuffer.clear();
	while (!m_outputQueue.empty())
		m_outputQueue.pop();

	return true;
}

// ===== Run 分发 =====
bool AtoD_M_Block::Run()
{
	if (IsVariableStepMode())
		return TimeDrivenRun();
	else
		return DataStreamRun();
}

// ===== DataStreamRun（完全内联） =====
bool AtoD_M_Block::DataStreamRun()
{
	std::string A_inPortName  = GetInputPortName(0);
	std::string A_outPortName = GetOutputPortName(0);
	std::string D_IPortName   = GetOutputPortName(1);
	std::string D_QPortName   = GetOutputPortName(2);

	auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeMatrix>(A_inPortName);
	if (inputData.empty())
		return false;

	const EnvelopeMatrix& in0 = inputData[0];
	const std::size_t rows     = in0.NumRows();
	const std::size_t cols     = in0.NumColumns();
	const std::size_t elemCount = in0.NumElements();

	const double fs = m_simulator_param.samplingRate;
	double t = m_simulator_param.startTime +
		static_cast<double>(m_atod_m->GetCount()) / std::max(fs, 1.0);

	ensure_state_count_(elemCount);

	std::vector<std::complex<double>> sampled(elemCount, std::complex<double>(0.0, 0.0));

	if (m_ConversionType == AtoD_M::Downsampled)
	{
		sampled = get_downsampled_matrix_(inputData, elemCount);
	}
	else
	{
		for (std::size_t i = 0; i < elemCount; ++i)
		{
			const std::complex<double> xin = in0(i).complex();
			sampled[i] = get_clocked_input_(xin, t, m_clockStates[i]);
		}
	}

	EnvelopeMatrix outA;
	IntMatrix      outI;
	IntMatrix      outQ;
	outA.Resize(rows, cols);
	outI.Resize(rows, cols);
	outQ.Resize(rows, cols);

	for (std::size_t i = 0; i < elemCount; ++i)
	{
		std::complex<double> xd = apply_distortion_(sampled[i], t, m_distortionStates[i]);

		QuantResult qi = quantize_(xd.real());
		QuantResult qq = quantize_(xd.imag());

		outA(i) = std::complex<double>(qi.analog, qq.analog);
		outI(i) = qi.codeDigital;
		outQ(i) = qq.codeDigital;

		m_distortionStates[i].lastInput     = sampled[i];
		m_distortionStates[i].lastInputTime = t;
		m_distortionStates[i].hasLastInput  = true;
	}

	WriteOutputData(A_outPortName, std::vector<EnvelopeMatrix>{ outA });
	WriteOutputData(D_IPortName,   std::vector<IntMatrix>{ outI });
	WriteOutputData(D_QPortName,   std::vector<IntMatrix>{ outQ });
	m_atod_m->Advance();

	++m_sampleIndex;
	return true;
}

// ===== TimeDrivenRun（三段式） =====
bool AtoD_M_Block::TimeDrivenRun()
{
	// ==== Stage 1：累积输入 ====
	{
		auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeMatrix>(GetInputPortName(0));
		for (auto& mat : inputData)
			m_inputBuffer.push_back({ mat });
	}

	const int neededFrames = (m_ConversionType == AtoD_M::Downsampled)
		? std::max(1, m_DownsampleFactor)
		: 1;

	// ==== Stage 2：处理所有可就绪的帧 ====
	while (static_cast<int>(m_inputBuffer.size()) >= neededFrames)
	{
		std::vector<EnvelopeMatrix> processVec;
		for (int k = 0; k < neededFrames; ++k)
		{
			processVec.push_back(m_inputBuffer.front().matrix);
			m_inputBuffer.erase(m_inputBuffer.begin());
		}

		const EnvelopeMatrix& in0 = processVec[0];
		const std::size_t rows     = in0.NumRows();
		const std::size_t cols     = in0.NumColumns();
		const std::size_t elemCount = in0.NumElements();

		const double fs = m_simulator_param.samplingRate;
		double t = m_simulator_param.startTime +
			static_cast<double>(m_atod_m->GetCount()) / std::max(fs, 1.0);

		ensure_state_count_(elemCount);

		std::vector<std::complex<double>> sampled(elemCount, std::complex<double>(0.0, 0.0));

		if (m_ConversionType == AtoD_M::Downsampled)
		{
			sampled = get_downsampled_matrix_(processVec, elemCount);
		}
		else
		{
			for (std::size_t i = 0; i < elemCount; ++i)
			{
				const std::complex<double> xin = in0(i).complex();
				sampled[i] = get_clocked_input_(xin, t, m_clockStates[i]);
			}
		}

		EnvelopeMatrix outA;
		IntMatrix      outI;
		IntMatrix      outQ;
		outA.Resize(rows, cols);
		outI.Resize(rows, cols);
		outQ.Resize(rows, cols);

		for (std::size_t i = 0; i < elemCount; ++i)
		{
			std::complex<double> xd = apply_distortion_(sampled[i], t, m_distortionStates[i]);

			QuantResult qi = quantize_(xd.real());
			QuantResult qq = quantize_(xd.imag());

			outA(i) = std::complex<double>(qi.analog, qq.analog);
			outI(i) = qi.codeDigital;
			outQ(i) = qq.codeDigital;

			m_distortionStates[i].lastInput     = sampled[i];
			m_distortionStates[i].lastInputTime = t;
			m_distortionStates[i].hasLastInput  = true;
		}

		m_outputQueue.push(OutputFrame{ outA, outI, outQ });
		++m_sampleIndex;
	}

	// ==== Stage 3：出队输出 ====
	if (!m_outputQueue.empty())
	{
		OutputFrame& frame = m_outputQueue.front();
		WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeMatrix>{ frame.A_out });
		WriteOutputData(GetOutputPortName(1), std::vector<IntMatrix>{ frame.D_I });
		WriteOutputData(GetOutputPortName(2), std::vector<IntMatrix>{ frame.D_Q });
		m_outputQueue.pop();
		m_atod_m->Advance();
	}

	return true;
}

// ===================================================================
//  算法内联 helper
// ===================================================================

// --------- 参数处理 ---------
void AtoD_M_Block::clamp_params_()
{
	m_nbits = clamp_int_(m_NBits, 4, 16);

	if (m_VRef <= 0.0)
		m_vref = 1.0;
	else
		m_vref = m_VRef;

	m_codeCount = 1 << m_nbits;
	m_midCode   = m_codeCount / 2;
	m_lsb       = 2.0 * m_vref / static_cast<double>(m_codeCount);

	if (m_DownsampleFactor < 1)
		m_DownsampleFactor = 1;

	if (m_DownsamplePhase < 0)
		m_DownsamplePhase = 0;
	if (m_DownsamplePhase >= m_DownsampleFactor)
		m_DownsamplePhase = m_DownsampleFactor - 1;

	if (m_Clock <= 0.0)
		m_Clock = 0.2e6;

	if (m_RJrms < 0.0)
		m_RJrms = 0.0;

	if (m_DNL < 0.0)
		m_DNL = 0.0;

	if (m_DNL <= 0.0)
		m_INL = 0.0;

	if (m_INL < m_DNL / 2.0)
		m_INL = m_DNL / 2.0;

	if (m_ENOB < 1.0)  m_ENOB = 1.0;
	if (m_ENOB > 16.0) m_ENOB = 16.0;

	m_ExcessBW = clip_(m_ExcessBW, 0.0, 1.0);
}

void AtoD_M_Block::build_transfer_table_()
{
	m_thresholds.assign(m_codeCount + 1, 0.0);
	m_levels.assign(m_codeCount, 0.0);

	m_thresholds[0] = -m_vref;
	m_thresholds[m_codeCount] = m_vref;

	const bool useNonlinear =
		(m_DistortionModel == AtoD_M::Jitter_INL_DNL) &&
		(m_DNL > 0.0 || m_INL > 0.0);

	if (!useNonlinear)
	{
		for (int i = 1; i < m_codeCount; ++i)
			m_thresholds[i] = -m_vref + static_cast<double>(i) * m_lsb;
		for (int i = 0; i < m_codeCount; ++i)
			m_levels[i] = -m_vref + (static_cast<double>(i) + 0.5) * m_lsb;
		return;
	}

	// DNL / INL 非线性建表
	std::vector<double> widths(m_codeCount, m_lsb);

	unsigned int local = 0xA5A5A5A5U ^ static_cast<unsigned int>(m_nbits * 131U);
	for (int i = 0; i < m_codeCount; ++i)
	{
		local ^= (local << 13);
		local ^= (local >> 17);
		local ^= (local << 5);

		double u = (static_cast<double>(local) + 1.0) / 4294967297.0;
		double e = (2.0 * u - 1.0) * m_DNL;
		e = clip_(e, -0.95, 0.95);
		widths[i] = m_lsb * (1.0 + e);
	}

	double sumW = 0.0;
	for (int i = 0; i < m_codeCount; ++i)
		sumW += widths[i];

	double scale = (2.0 * m_vref) / std::max(sumW, kTiny);

	m_thresholds[0] = -m_vref;
	for (int i = 1; i < m_codeCount; ++i)
		m_thresholds[i] = m_thresholds[i - 1] + widths[i - 1] * scale;
	m_thresholds[m_codeCount] = m_vref;

	for (int i = 0; i < m_codeCount; ++i)
	{
		double center = 0.5 * (m_thresholds[i] + m_thresholds[i + 1]);
		double u = static_cast<double>(i) / static_cast<double>(std::max(m_codeCount - 1, 1));
		double inlOffset = m_INL * m_lsb * std::sin(2.0 * kPi * u);
		m_levels[i] = clip_(center + inlOffset, -m_vref + 0.5 * m_lsb, m_vref - 0.5 * m_lsb);
	}
}

void AtoD_M_Block::reset_states_()
{
	m_clockStates.clear();
	m_distortionStates.clear();
}

void AtoD_M_Block::ensure_state_count_(std::size_t n)
{
	if (m_clockStates.size() != n)
		m_clockStates.assign(n, ClockState());
	if (m_distortionStates.size() != n)
		m_distortionStates.assign(n, DistortionState());
}

// --------- 输入采样 ---------
std::complex<double> AtoD_M_Block::read_matrix_sample_(const EnvelopeMatrix& m, std::size_t elem)
{
	if (elem >= m.NumElements())
		return std::complex<double>(0.0, 0.0);
	return m(elem).complex();
}

std::vector<std::complex<double>> AtoD_M_Block::get_downsampled_matrix_(
	const std::vector<EnvelopeMatrix>& inputVec, std::size_t elemCount)
{
	std::vector<std::complex<double>> out(elemCount, std::complex<double>(0.0, 0.0));

	int factor = std::max(1, m_DownsampleFactor);
	int phase  = clamp_int_(m_DownsamplePhase, 0, factor - 1);

	if (m_AntiAliasingFilter != AtoD_M::AA_ON || factor <= 1)
	{
		if (static_cast<int>(inputVec.size()) > phase)
		{
			for (std::size_t i = 0; i < elemCount; ++i)
				out[i] = read_matrix_sample_(inputVec[phase], i);
		}
		return out;
	}

	// 抗混叠近似：带余弦窗的加权平均
	std::vector<std::complex<double>> acc(elemCount, std::complex<double>(0.0, 0.0));
	double wsum = 0.0;
	int limit = std::min(factor, static_cast<int>(inputVec.size()));

	for (int k = 0; k < limit; ++k)
	{
		double x = 0.0;
		if (factor > 1)
			x = static_cast<double>(k) / static_cast<double>(factor - 1);
		double w = 0.5 - 0.5 * std::cos(2.0 * kPi * x);
		w = (1.0 - m_ExcessBW) + m_ExcessBW * w;

		for (std::size_t i = 0; i < elemCount; ++i)
			acc[i] += w * read_matrix_sample_(inputVec[k], i);

		wsum += w;
	}

	if (wsum <= kTiny)
	{
		if (static_cast<int>(inputVec.size()) > phase)
		{
			for (std::size_t i = 0; i < elemCount; ++i)
				out[i] = read_matrix_sample_(inputVec[phase], i);
		}
		return out;
	}

	for (std::size_t i = 0; i < elemCount; ++i)
		out[i] = acc[i] / wsum;

	return out;
}

double AtoD_M_Block::first_positive_crossing_at_or_after_(double t) const
{
	const double period    = 1.0 / std::max(m_Clock, kTiny);
	const double phaseRad  = m_Phase * kPi / 180.0;
	const double base      = ((1.5 * kPi) - phaseRad) / (2.0 * kPi * std::max(m_Clock, kTiny));

	double k = std::ceil((t - base) / period - 1e-12);
	if (k < 0.0) k = 0.0;

	double ts = base + k * period;
	while (ts < t - 1e-15)
		ts += period;

	return ts;
}

std::complex<double> AtoD_M_Block::interp_(const std::complex<double>& x0, double t0,
	const std::complex<double>& x1, double t1, double ts) const
{
	if (std::fabs(t1 - t0) <= kTiny)
		return x1;
	double a = (ts - t0) / (t1 - t0);
	a = clip_(a, 0.0, 1.0);
	return x0 + (x1 - x0) * a;
}

std::complex<double> AtoD_M_Block::get_clocked_input_(const std::complex<double>& x, double t, ClockState& st)
{
	if (m_Clock <= 0.0)
		return x;

	const double phaseRad = m_Phase * kPi / 180.0;
	const double c = std::cos(2.0 * kPi * m_Clock * t + phaseRad);
	const double period = 1.0 / std::max(m_Clock, kTiny);

	if (!st.hasClockState)
	{
		st.heldSample    = std::complex<double>(0.0, 0.0);
		st.hasPendingClockSample = false;
		st.pendingClockSample    = std::complex<double>(0.0, 0.0);
		st.lastClockValue        = c;
		st.hasClockState         = true;

		st.hasRawInputState = true;
		st.prevRawInputTime = t;
		st.prevRawInput     = x;

		st.nextClockCrossingTime = first_positive_crossing_at_or_after_(t);
		st.hasNextClockCrossing  = true;

		if (st.nextClockCrossingTime <= t + 1e-15)
		{
			st.pendingClockSample    = x;
			st.hasPendingClockSample = true;
			st.nextClockCrossingTime += period;
		}

		return st.heldSample;
	}

	if (st.hasPendingClockSample)
	{
		st.heldSample            = st.pendingClockSample;
		st.hasPendingClockSample = false;
	}

	const std::complex<double> y = st.heldSample;

	if (!st.hasRawInputState)
	{
		st.hasRawInputState = true;
		st.prevRawInputTime = t;
		st.prevRawInput     = x;
		st.lastClockValue   = c;
		return y;
	}

	if (t < st.prevRawInputTime - 1e-15)
	{
		st.prevRawInputTime       = t;
		st.prevRawInput           = x;
		st.nextClockCrossingTime  = first_positive_crossing_at_or_after_(t);
		st.hasNextClockCrossing   = true;
		st.lastClockValue         = c;
		return y;
	}

	if (!st.hasNextClockCrossing)
	{
		st.nextClockCrossingTime = first_positive_crossing_at_or_after_(st.prevRawInputTime);
		st.hasNextClockCrossing  = true;
	}

	while (st.nextClockCrossingTime <= t + 1e-15)
	{
		if (st.nextClockCrossingTime >= st.prevRawInputTime - 1e-15)
		{
			st.pendingClockSample    = interp_(st.prevRawInput, st.prevRawInputTime, x, t, st.nextClockCrossingTime);
			st.hasPendingClockSample = true;
		}
		st.nextClockCrossingTime += period;
	}

	st.prevRawInputTime = t;
	st.prevRawInput     = x;
	st.lastClockValue   = c;

	return y;
}

// --------- 失真模型 ---------
std::complex<double> AtoD_M_Block::apply_distortion_(const std::complex<double>& x, double t, DistortionState& st)
{
	std::complex<double> y = x;

	if (m_DistortionModel == AtoD_M::Distortion_None)
		return y;

	if (m_DistortionModel == AtoD_M::Jitter_INL_DNL)
	{
		if (m_EnableJitter == AtoD_M::Time_Domain)
			y = apply_jitter_(y, t, st);
		else if (m_EnableJitter == AtoD_M::Frequency_Domain)
			y = apply_phase_noise_(y);
		return y;
	}

	if (m_DistortionModel == AtoD_M::ENOB_value)
	{
		y = apply_noise_(y, target_snr_db_());
		return y;
	}

	if (m_DistortionModel == AtoD_M::SNR_and_Harmonics)
	{
		y = apply_harmonics_(y, m_H2_dBc, m_H3_dBc, m_H4_dBc, m_H5_dBc);
		y = apply_noise_(y, target_snr_db_());
		return y;
	}

	if (m_DistortionModel == AtoD_M::SINAD_and_SFDR)
	{
		y = apply_sinad_sfdr_(y);
		y = apply_noise_(y, target_snr_db_());
		return y;
	}

	return y;
}

std::complex<double> AtoD_M_Block::apply_jitter_(const std::complex<double>& x, double t, DistortionState& st)
{
	if (m_RJrms <= 0.0 || !st.hasLastInput)
		return x;

	double dt = t - st.lastInputTime;
	if (std::fabs(dt) <= kTiny)
		return x;

	double jitter = m_RJrms * gaussian_();
	jitter = clip_(jitter, -3.0 * m_RJrms, 3.0 * m_RJrms);

	std::complex<double> slope = (x - st.lastInput) / dt;
	return x + slope * jitter;
}

std::complex<double> AtoD_M_Block::apply_phase_noise_(const std::complex<double>& x)
{
	double sigma = 0.0;

	if (!m_PhaseNoiseVector.empty() && m_PhaseNoiseVector.size() >= 2)
	{
		double acc = 0.0;
		int pairs = static_cast<int>(m_PhaseNoiseVector.size()) / 2;
		for (int i = 0; i < pairs; ++i)
		{
			double ldbc = m_PhaseNoiseVector[2 * i + 1];
			acc += std::pow(10.0, ldbc / 10.0);
		}
		sigma = std::sqrt(std::max(acc, 0.0)) * 1e-3;
	}

	if (sigma <= 0.0)
		return x;

	double ph = sigma * gaussian_();

	if (m_PN_Type == AtoD_M::Fixed_freq_offset)
		ph = sigma;
	else if (m_PN_Type == AtoD_M::Fixed_freq_offset_and_amplitude)
		ph = sigma * std::sin(2.0 * kPi * static_cast<double>(m_sampleIndex) / 1024.0);

	return x * std::complex<double>(std::cos(ph), std::sin(ph));
}

std::complex<double> AtoD_M_Block::apply_harmonics_(const std::complex<double>& x,
	double h2_dBc, double h3_dBc, double h4_dBc, double h5_dBc) const
{
	double i = apply_harmonics_real_(x.real(), h2_dBc, h3_dBc, h4_dBc, h5_dBc);
	double q = apply_harmonics_real_(x.imag(), h2_dBc, h3_dBc, h4_dBc, h5_dBc);
	return std::complex<double>(i, q);
}

std::complex<double> AtoD_M_Block::apply_sinad_sfdr_(const std::complex<double>& x) const
{
	double sfdr = m_SFDR_dBc;
	if (sfdr < m_SINAD_dB + 6.0)
		sfdr = m_SINAD_dB + 6.0;
	if (sfdr > m_SINAD_dB + 20.0)
		return x;

	double h2 = -std::fabs(sfdr);
	return apply_harmonics_(x, h2, h2 - 3.0, -400.0, -400.0);
}

double AtoD_M_Block::apply_harmonics_real_(double x,
	double h2_dBc, double h3_dBc, double h4_dBc, double h5_dBc) const
{
	double A = std::max(full_scale_peak_(), std::fabs(x));
	A = std::max(A, kTiny);
	double u = clip_(x / A, -1.0, 1.0);

	double T2 = 2.0 * u * u - 1.0;
	double T3 = 4.0 * u * u * u - 3.0 * u;
	double u2 = u * u;
	double u3 = u2 * u;
	double u4 = u2 * u2;
	double u5 = u4 * u;
	double T4 = 8.0 * u4 - 8.0 * u2 + 1.0;
	double T5 = 16.0 * u5 - 20.0 * u3 + 5.0 * u;

	double y = x;
	if (h2_dBc > -300.0) y += A * db_to_amp_(h2_dBc) * T2;
	if (h3_dBc > -300.0) y += A * db_to_amp_(h3_dBc) * T3;
	if (h4_dBc > -300.0) y += A * db_to_amp_(h4_dBc) * T4;
	if (h5_dBc > -300.0) y += A * db_to_amp_(h5_dBc) * T5;
	return y;
}

std::complex<double> AtoD_M_Block::apply_noise_(const std::complex<double>& x, double snr_dB)
{
	if (snr_dB > 250.0)
		return x;

	double sigRms = std::max(std::abs(x) / std::sqrt(2.0),
		(m_vref / std::sqrt(2.0)) * db_to_amp_(m_Level_dBFS));

	double noiseRms = sigRms * db_to_amp_(-snr_dB);
	if (noiseRms <= 0.0)
		return x;

	double ni = noiseRms * gaussian_();
	double nq = noiseRms * gaussian_();
	return x + std::complex<double>(ni, nq);
}

// --------- 量化 ---------
AtoD_M_Block::QuantResult AtoD_M_Block::quantize_(double x) const
{
	QuantResult r;

	double xc = clip_(x, -m_vref, m_vref);

	const bool idealUniformTable =
		!(m_DistortionModel == AtoD_M::Jitter_INL_DNL && (m_DNL > 0.0 || m_INL > 0.0));

	if (idealUniformTable && !m_thresholds.empty())
	{
		const double edgeTol = m_lsb * 5.0e-3;
		for (int k = 1; k < m_codeCount; ++k)
		{
			const double th = m_thresholds[k];
			if (xc < th && (th - xc) <= edgeTol)
			{
				xc = th;
				break;
			}
		}
	}

	int code = 0;

	if (m_thresholds.empty() || m_levels.empty())
	{
		double u = (xc + m_vref) / m_lsb;
		code = static_cast<int>(std::floor(u));
		code = clamp_int_(code, 0, m_codeCount - 1);
		r.analog = -m_vref + (static_cast<double>(code) + 0.5) * m_lsb;
	}
	else
	{
		if (xc <= m_thresholds.front())
			code = 0;
		else if (xc >= m_thresholds.back())
			code = m_codeCount - 1;
		else
		{
			auto it = std::upper_bound(m_thresholds.begin(), m_thresholds.end(), xc);
			code = static_cast<int>((it - m_thresholds.begin()) - 1);
			code = clamp_int_(code, 0, m_codeCount - 1);
		}
		r.analog = m_levels[code];
	}

	r.codeOffset = code;

	if (m_OutputDigitalFormat == AtoD_M::Twos_complement)
		r.codeDigital = code - m_midCode;
	else
		r.codeDigital = code;

	return r;
}

// --------- SNR / 工具函数 ---------
double AtoD_M_Block::target_snr_db_() const
{
	double idealSNR = 6.02 * static_cast<double>(m_nbits) + 1.76;

	if (m_DistortionModel == AtoD_M::ENOB_value)
	{
		double snr = 6.02 * m_ENOB + 1.76;
		if (snr > idealSNR + std::fabs(m_Level_dBFS))
			snr = idealSNR + std::fabs(m_Level_dBFS);
		return snr;
	}

	if (m_DistortionModel == AtoD_M::SNR_and_Harmonics)
	{
		double snr = m_SNR_dB;
		if (snr > idealSNR + std::fabs(m_Level_dBFS))
			snr = idealSNR + std::fabs(m_Level_dBFS);
		return snr;
	}

	if (m_DistortionModel == AtoD_M::SINAD_and_SFDR)
	{
		double snr = m_SINAD_dB;
		if (snr > idealSNR + std::fabs(m_Level_dBFS))
			snr = idealSNR + std::fabs(m_Level_dBFS);
		return snr;
	}

	return 300.0;
}

double AtoD_M_Block::full_scale_peak_() const
{
	return std::max(m_vref * db_to_amp_(m_Level_dBFS), kTiny);
}

double AtoD_M_Block::uniform_()
{
	m_rngState ^= (m_rngState << 13);
	m_rngState ^= (m_rngState >> 17);
	m_rngState ^= (m_rngState << 5);
	return (static_cast<double>(m_rngState) + 1.0) / 4294967297.0;
}

double AtoD_M_Block::gaussian_()
{
	double u1 = std::max(uniform_(), 1e-12);
	double u2 = std::max(uniform_(), 1e-12);
	double r  = std::sqrt(-2.0 * std::log(u1));
	double th = 2.0 * kPi * u2;
	return r * std::cos(th);
}

double AtoD_M_Block::db_to_amp_(double dB)
{
	return std::pow(10.0, dB / 20.0);
}

double AtoD_M_Block::clip_(double x, double lo, double hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}

int AtoD_M_Block::clamp_int_(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

// ===== ConvertStringTo* =====
AtoD_M::OutputDigitalFormatEnum AtoD_M_Block::ConvertStringToOutputDigitalFormatEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "offset_binary")  return AtoD_M::Offset_binary;
	if (lower == "twos_complement" || lower == "twos-complement" || lower == "1")
		return AtoD_M::Twos_complement;
	return AtoD_M::Offset_binary;
}

AtoD_M::DistortionModelEnum AtoD_M_Block::ConvertStringToDistortionModelEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "none" || lower == "distortion_none") return AtoD_M::Distortion_None;
	if (lower == "jitter/inl/dnl" || lower == "jitter_inl_dnl" || lower == "1")
		return AtoD_M::Jitter_INL_DNL;
	if (lower == "enob_value" || lower == "enob value" || lower == "2")
		return AtoD_M::ENOB_value;
	if (lower == "snr_and_harmonics" || lower == "snr and harmonics" || lower == "3")
		return AtoD_M::SNR_and_Harmonics;
	if (lower == "sinad_and_sfdr" || lower == "sinad and sfdr" || lower == "4")
		return AtoD_M::SINAD_and_SFDR;
	return AtoD_M::Distortion_None;
}

AtoD_M::EnableJitterEnum AtoD_M_Block::ConvertStringToEnableJitterEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "no" || lower == "jitter_no")  return AtoD_M::Jitter_No;
	if (lower == "time_domain" || lower == "time domain" || lower == "1")
		return AtoD_M::Time_Domain;
	if (lower == "frequency_domain" || lower == "frequency domain" || lower == "2")
		return AtoD_M::Frequency_Domain;
	return AtoD_M::Jitter_No;
}

AtoD_M::PN_TypeEnum AtoD_M_Block::ConvertStringToPN_TypeEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "random_pn" || lower == "random pn") return AtoD_M::Random_PN;
	if (lower == "fixed_freq_offset" || lower == "fixed freq offset" || lower == "1")
		return AtoD_M::Fixed_freq_offset;
	if (lower == "fixed_freq_offset_and_amplitude" || lower == "fixed freq offset and amplitude" || lower == "2")
		return AtoD_M::Fixed_freq_offset_and_amplitude;
	return AtoD_M::Random_PN;
}

AtoD_M::FFT_SizeEnum AtoD_M_Block::ConvertStringToFFT_SizeEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "2^12" || lower == "fft_2_12") return AtoD_M::FFT_2_12;
	if (lower == "2^13" || lower == "fft_2_13" || lower == "1") return AtoD_M::FFT_2_13;
	if (lower == "2^14" || lower == "fft_2_14" || lower == "2") return AtoD_M::FFT_2_14;
	if (lower == "2^15" || lower == "fft_2_15" || lower == "3") return AtoD_M::FFT_2_15;
	if (lower == "2^16" || lower == "fft_2_16" || lower == "4") return AtoD_M::FFT_2_16;
	return AtoD_M::FFT_2_12;
}

AtoD_M::SNR_ModelEnum AtoD_M_Block::ConvertStringToSNR_ModelEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "quantization_and_jitter") return AtoD_M::Quantization_and_Jitter;
	if (lower == "quantization_and_inl_dnl" || lower == "1")
		return AtoD_M::Quantization_and_INL_DNL;
	if (lower == "quantization_and_jitter_or_inl_dnl" || lower == "2")
		return AtoD_M::Quantization_and_Jitter_or_INL_DNL;
	if (lower == "quantization_jitter_and_thermal_noise" || lower == "3")
		return AtoD_M::Quantization_Jitter_and_Thermal_Noise;
	return AtoD_M::Quantization_and_Jitter;
}

AtoD_M::ConversionTypeEnum AtoD_M_Block::ConvertStringToConversionTypeEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "clocked")  return AtoD_M::Clocked;
	if (lower == "downsampled" || lower == "1") return AtoD_M::Downsampled;
	return AtoD_M::Clocked;
}

AtoD_M::AntiAliasingFilterEnum AtoD_M_Block::ConvertStringToAntiAliasingFilterEnum(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "off" || lower == "aa_off") return AtoD_M::AA_OFF;
	if (lower == "on" || lower == "aa_on" || lower == "1") return AtoD_M::AA_ON;
	return AtoD_M::AA_OFF;
}
