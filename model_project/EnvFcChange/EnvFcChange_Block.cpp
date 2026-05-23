#include "EnvFcChange_Block.h"
#include <complex>
#include <cmath>

EnvFcChange_Block::EnvFcChange_Block(const std::string& name)
	: Block(name)
{
}

void EnvFcChange_Block::SetDefaultParamters()
{
	m_outputFc = 0.0;
	m_bandwidth = 0.0;

	m_ts = 0.0;
	m_lastTime = std::numeric_limits<double>::quiet_NaN();
	m_alpha = 0.0;
	m_iLp = 0.0;
	m_qLp = 0.0;
}

void EnvFcChange_Block::SetParameters(double outputFc, double bandwidth)
{
	m_outputFc = outputFc;
	m_bandwidth = bandwidth;
	if (m_envFcChange) {
		m_envFcChange->OutputFc = outputFc;
		m_envFcChange->Bandwidth = bandwidth;
	}
}

bool EnvFcChange_Block::Setup()
{
	Block::Setup();
	return true;
}

bool EnvFcChange_Block::Run()
{
	std::string inputPort = GetInputPortName(0);
	std::string outputPort = GetOutputPortName(0);

	auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
	if (inputData.empty()) {
		return true;
	}
	UpdateCharacterizationFrequency();

	const double f2 = m_outputFc;
	if (GetOutputPort(outputPort)->getCharacterizationFrequency() != f2) {
		GetOutputPort(outputPort)->setCharacterizationFrequency(f2);
	}

	const SimuParameter simulator_param = getSimu();
	const double tOut = (simulator_param.samplingRate > 0.0)
		? (simulator_param.startTime + static_cast<double>(m_envFcChange->GetCount()) / simulator_param.samplingRate)
		: 0.0;
	if (std::isfinite(m_lastTime)) {
		m_ts = std::max(0.0, tOut - m_lastTime);
	}
	m_lastTime = tOut;

	const double f1 = GetInputPort(inputPort)->getCharacterizationFrequency();

	std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
	outputData.reserve(inputData.size());

	for (const auto& x : inputData) {
		SystemVueModelBuilder::EnvelopeSignal y;

		if (f1 > 0.0) {
			std::complex<double> cx = x.ConvertToNewFc(f1, f2, tOut);
			CopyToEnvelopeSignal(cx, y);
		}
		else {
			if (f2 <= 0.0) {
				CopyToEnvelopeSignal(std::complex<double>(x.real(), 0.0), y);
			}
			else {
				if (m_ts > 0.0) {
					double BW = (m_bandwidth > 0.0 ? m_bandwidth : f2);
					if (BW > 0.0) {
						const double fc_lp = 0.5 * BW;
						const double a = std::exp(-2.0 * EnvFcChange::kPI * fc_lp * m_ts);
						m_alpha = clip(a, 0.0, 0.999999);
					}
					else {
						m_alpha = 0.0;
					}
				}
				const double xr = x.real();
				const double c = std::cos(2.0 * EnvFcChange::kPI * f2 * tOut);
				const double s = std::sin(2.0 * EnvFcChange::kPI * f2 * tOut);

				const double i_raw = xr * c;
				const double q_raw = xr * (-s);

				const double one_ma = 1.0 - m_alpha;
				m_iLp = one_ma * i_raw + m_alpha * m_iLp;
				m_qLp = one_ma * q_raw + m_alpha * m_qLp;

				CopyToEnvelopeSignal(std::complex<double>(m_iLp, m_qLp), y);
			}
		}

		outputData.push_back(y);
	}

	WriteOutputData(outputPort, outputData);

	if (m_envFcChange) {
		m_envFcChange->Advance();
	}

	return true;
}

bool EnvFcChange_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_envFcChange = std::make_unique<EnvFcChange>();

	AddInputPort("input", m_envFcChange->input, 1, Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("output", m_envFcChange->output, 1, Block::DataType::ENVELOPE_SIGNAL);

	SetDefaultParamters();

	try { m_outputFc = std::stod(getParameter("OutputFc").Value); } catch (...) {}
	try { m_bandwidth = std::stod(getParameter("Bandwidth").Value); } catch (...) {}

	SetParameters(m_outputFc, m_bandwidth);

	return true;
}

void EnvFcChange_Block::UpdateCharacterizationFrequency()
{
	if (m_envFcChange) {
		m_envFcChange->PropagateCharacterizationFrequency();
		GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(
			GetInputPort(GetInputPortName(0))->getCharacterizationFrequency());
	}
}




