#include "Demodulator_Block.h"
#include <algorithm>

Demodulator_Block::Demodulator_Block(const std::string& name)
	: Block(name)
{
}

void Demodulator_Block::SetDefaultParamters()
{
	m_outputType = Demodulator::OT_IQ;
	m_ampSensitivity = 1.0;
	m_phaseSensitivity = 1.0 / 90.0;
	m_freqSensitivity = 1.0e-4;
	m_fCarrier = 0.2e6;
	m_initialPhase = 0.0;
	m_mirrorSignal = Demodulator::Mirror_No;
	m_showIQImpairments = Demodulator::IQImp_No;
	m_gainImbalance = 0.0;
	m_phaseImbalance = 0.0;
	m_iOriginOffset = 0.0;
	m_qOriginOffset = 0.0;
	m_iqRotation = 0.0;
	m_prevThetaRad = 0.0;
	m_prevTime = 0.0;
	m_havePrev = false;
}

void Demodulator_Block::SetParameters()
{
	if (m_demodulator) {
		m_demodulator->OutputType = m_outputType;
		m_demodulator->AmpSensitivity = m_ampSensitivity;
		m_demodulator->PhaseSensitivity = m_phaseSensitivity;
		m_demodulator->FreqSensitivity = m_freqSensitivity;
		m_demodulator->FCarrier = m_fCarrier;
		m_demodulator->InitialPhase = m_initialPhase;
		m_demodulator->MirrorSignal = m_mirrorSignal;
		m_demodulator->ShowIQ_Impairments = m_showIQImpairments;
		m_demodulator->GainImbalance = m_gainImbalance;
		m_demodulator->PhaseImbalance = m_phaseImbalance;
		m_demodulator->I_OriginOffset = m_iOriginOffset;
		m_demodulator->Q_OriginOffset = m_qOriginOffset;
		m_demodulator->IQ_Rotation = m_iqRotation;
	}
}

bool Demodulator_Block::Setup()
{
	Block::Setup();
	return true;
}

bool Demodulator_Block::Run()
{
	std::string inputPort = GetInputPortName(0);
	std::string output1Port = GetOutputPortName(0);
	std::string output2Port = GetOutputPortName(1);

	auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
	if (inputData.empty()) {
		return true;
	}

	std::vector<double> out1;
	std::vector<double> out2;
	out1.reserve(inputData.size());
	out2.reserve(inputData.size());

	const SimuParameter simulator_param = getSimu();
	const double fs = simulator_param.samplingRate;
	const double baseCount = static_cast<double>(GetCount());

	for (size_t i = 0; i < inputData.size(); ++i) {
		const auto& sNow = inputData[i];
		const double tNow = (fs > 0.0)
			? (simulator_param.startTime + (baseCount + static_cast<double>(i)) / fs)
			: 0.0;

		double I_now = 0.0, Q_now = 0.0;
		m_demodulator->processEnvelopeSample(sNow, tNow, I_now, Q_now);

		const double amp = std::hypot(I_now, Q_now);
		const double thetaRaw = std::atan2(Q_now, I_now);
		const double thetaUnwr = m_demodulator->unwrapPhase(thetaRaw);

		double freqRadPerSec = 0.0;
		if (m_havePrev) {
			const double dth = thetaUnwr - m_prevThetaRad;
			const double dt = tNow - m_prevTime;
			if (dt > 0.0) freqRadPerSec = dth / dt;
		}

		double y1 = 0.0, y2 = 0.0;
		switch (m_outputType) {
		case Demodulator::OT_IQ:
			y1 = m_ampSensitivity * I_now;
			y2 = m_ampSensitivity * Q_now;
			break;
		case Demodulator::OT_AmpPhase:
			y1 = m_ampSensitivity * amp;
			y2 = m_phaseSensitivity * (thetaUnwr * 180.0 / Demodulator::kPI);
			break;
		case Demodulator::OT_AmpFreq:
			y1 = m_ampSensitivity * amp;
			y2 = m_freqSensitivity * (freqRadPerSec / (2.0*Demodulator::kPI));
			break;
		}

		out1.push_back(y1);
		out2.push_back(y2);

		m_prevThetaRad = thetaUnwr;
		m_prevTime = tNow;
		m_havePrev = true;
	}

	WriteOutputData(output1Port, out1);
	WriteOutputData(output2Port, out2);
	Advance();

	return true;
}

bool Demodulator_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_demodulator = std::make_unique<Demodulator>();

	AddInputPort("input", m_demodulator->input, 1, Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("output1", m_demodulator->output1, 1, Block::DataType::TIMED_DOUBLE);
	AddOutputPort("output2", m_demodulator->output2, 1, Block::DataType::TIMED_DOUBLE);

	SetDefaultParamters();

	try { m_outputType = ConvertStringToOutputType(getParameter("OutputType").Value); } catch (...) {}
	try { m_fCarrier = std::stod(getParameter("FCarrier").Value); } catch (...) {}
	try { m_initialPhase = std::stod(getParameter("InitialPhase").Value); } catch (...) {}
	try { m_ampSensitivity = std::stod(getParameter("AmpSensitivity").Value); } catch (...) {}
	try { m_phaseSensitivity = std::stod(getParameter("PhaseSensitivity").Value); } catch (...) {}
	try { m_freqSensitivity = std::stod(getParameter("FreqSensitivity").Value); } catch (...) {}
	try { m_mirrorSignal = ConvertStringToMirror(getParameter("MirrorSignal").Value); } catch (...) {}
	try { m_showIQImpairments = ConvertStringToIQImp(getParameter("ShowIQ_Impairments").Value); } catch (...) {}
	try { m_gainImbalance = std::stod(getParameter("GainImbalance").Value); } catch (...) {}
	try { m_phaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) {}
	try { m_iOriginOffset = std::stod(getParameter("I_OriginOffset").Value); } catch (...) {}
	try { m_qOriginOffset = std::stod(getParameter("Q_OriginOffset").Value); } catch (...) {}
	try { m_iqRotation = std::stod(getParameter("IQ_Rotation").Value); } catch (...) {}

	SetParameters();

	m_demodulator->output1.SetRate(1U);
	m_demodulator->output2.SetRate(1U);
	m_demodulator->input.SetRate(1U);
	m_demodulator->input.SetStartTime(getSimu().startTime); // TODO: input not connected; timing setup may be unreliable

	return true;
}

Demodulator::OutputTypeEnum Demodulator_Block::ConvertStringToOutputType(const std::string& value)
{
	std::string trimmedValue;
	trimmedValue.reserve(value.size());
	for (char c : value) {
		if (!std::isspace(static_cast<unsigned char>(c))) {
			trimmedValue.push_back(c);
		}
	}
	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (lowerValue == "ot_iq") {
		return Demodulator::OT_IQ;
	} else if (lowerValue == "ot_ampphase") {
		return Demodulator::OT_AmpPhase;
	} else if (lowerValue == "ot_ampfreq") {
		return Demodulator::OT_AmpFreq;
	}

	if (lowerValue == "iq" || lowerValue == "i/q" || lowerValue == "0") {
		return Demodulator::OT_IQ;
	} else if (lowerValue == "ampphase" || lowerValue == "amp/phase" || lowerValue == "1") {
		return Demodulator::OT_AmpPhase;
	} else if (lowerValue == "ampfreq" || lowerValue == "amp/freq" || lowerValue == "2") {
		return Demodulator::OT_AmpFreq;
	}
	return Demodulator::OT_IQ;
}

Demodulator::MirrorEnum Demodulator_Block::ConvertStringToMirror(const std::string& value)
{
	std::string trimmedValue;
	trimmedValue.reserve(value.size());
	for (char c : value) {
		if (!std::isspace(static_cast<unsigned char>(c))) {
			trimmedValue.push_back(c);
		}
	}
	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (lowerValue == "mirror_no") {
		return Demodulator::Mirror_No;
	} else if (lowerValue == "mirror_yes") {
		return Demodulator::Mirror_Yes;
	}

	if (lowerValue == "no" || lowerValue == "0") {
		return Demodulator::Mirror_No;
	} else if (lowerValue == "yes" || lowerValue == "1") {
		return Demodulator::Mirror_Yes;
	}
	return Demodulator::Mirror_No;
}

Demodulator::IQImpEnum Demodulator_Block::ConvertStringToIQImp(const std::string& value)
{
	std::string trimmedValue;
	trimmedValue.reserve(value.size());
	for (char c : value) {
		if (!std::isspace(static_cast<unsigned char>(c))) {
			trimmedValue.push_back(c);
		}
	}
	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (lowerValue == "iqimp_no") {
		return Demodulator::IQImp_No;
	} else if (lowerValue == "iqimp_yes") {
		return Demodulator::IQImp_Yes;
	}
	return Demodulator::IQImp_No;
}

