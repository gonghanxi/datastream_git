#include "SubEnv_Block.h"
#include "TimedDFModel.h"
#include <complex>
#include <cmath>

SubEnv_Block::SubEnv_Block(const std::string& name)
	: Block(name)
{
}

bool SubEnv_Block::Setup()
{
	Block::Setup();
	return true;
}

bool SubEnv_Block::Run()
{;

	std::string negPortName = GetInputPortName(0);
	std::string posPortName = GetInputPortName(1);

    auto negData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(negPortName);
    auto posData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(posPortName);

	if (posData.empty()) {
		std::cout << "ERROR: No pos input data available" << std::endl;
		return false;
	}

	PropagateCharacterizationFrequency();
	double outputFc = fcOut;


    double dTime = getSimu().startTime + m_subEnv->GetCount() / getSimu().samplingRate;

	std::complex<double> acc(0.0, 0.0);
    for (size_t i = 0; i < posData.size(); ++i) {
        double inputFc = GetInputPort(posPortName)->getCharacterizationFrequency();
        std::complex<double> converted = posData[0].complex();
		if (inputFc != outputFc) {
			double phase = 2.0 * M_PI * (inputFc - outputFc) * dTime;
			converted *= std::complex<double>(std::cos(phase), std::sin(phase));
		}
		acc += converted;
	}

	for (size_t i = 0; i < negData.size(); ++i) {
        double inputFc = GetInputPort(negPortName)->GetBusConnections().at(static_cast<int>(i)).bridgeReader->getCharacterizationFrequency();
        std::complex<double> converted = negData[i].complex();
		if (inputFc != outputFc) {
			double phase = 2.0 * M_PI * (inputFc - outputFc) * dTime;
			converted *= std::complex<double>(std::cos(phase), std::sin(phase));
		}
		acc -= converted;
	}

	std::string outputPortName = GetOutputPortName(0);
    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
	outputData.push_back(acc);
	WriteOutputData(outputPortName, outputData);

    m_subEnv->Advance();

	return true;
}

bool SubEnv_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_subEnv = std::make_unique<SubEnv>();

    AddInputPort("neg", m_subEnv->neg, 1, DataType::ENVELOPE_BUS);
	AddInputPort("pos", m_subEnv->pos, 1, DataType::ENVELOPE_SIGNAL);
	AddOutputPort("output", m_subEnv->output, 1, DataType::ENVELOPE_SIGNAL);

	SetDefaultParameters();

	simulator_param = getSimu();

	m_FcOut = ConvertStringToSelectedFcOut(getParameter("FcOut").Value);
	m_UserDefinedFc = std::stod(getParameter("UserDefinedFc").Value);

	SetParameters(m_UserDefinedFc, m_FcOut);

	return true;
}

void SubEnv_Block::SetParameters(double userDefinedFc, SubEnv::SelectedFcOut fcOut)
{
	if (m_subEnv) {
		m_subEnv->UserDefinedFc = userDefinedFc;
		m_subEnv->FcOut = fcOut;
		m_subEnv->output.SetSampleRate(simulator_param.samplingRate);
	}
}

SubEnv::SelectedFcOut SubEnv_Block::ConvertStringToSelectedFcOut(const std::string& value)
{
	std::string trimmedValue = value;
	trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
	trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

	if (lowerValue == "min" || lowerValue == "0") {
		return SubEnv::min;
	} else if (lowerValue == "max" || lowerValue == "1") {
		return SubEnv::max;
	} else if (lowerValue == "center" || lowerValue == "2") {
		return SubEnv::center;
	} else if (lowerValue == "userdefined" || lowerValue == "3") {
		return SubEnv::userDefined;
	}
	return SubEnv::center;
}

void SubEnv_Block::SetDefaultParameters()
{
	m_UserDefinedFc = 100e6;
	m_FcOut = SubEnv::center;
}

void SubEnv_Block::PropagateCharacterizationFrequency()
{
	fcmax = 0.0;
	fcmean = 0.0;
	fcmin = GetInputPort("neg")->GetBusConnections().begin()->bridgeReader->getCharacterizationFrequency();

    int channelNumNeg = GetInputPort(GetInputPortName(0))->GetBusConnectionCount();
	for (int i = 0; i < channelNumNeg; i++)
	{
        fc = GetInputPort(GetInputPortName(0))->GetBusConnections().at(i).bridgeReader->getCharacterizationFrequency();
		fcmax = (fcmax < fc ? fc : fcmax);
		fcmin = (fcmin > fc ? fc : fcmin);
		fcmean += fc;
	}
	fcmean /= channelNumNeg;

	switch (m_FcOut)
	{
		case SubEnv::min: fcOut = fcmin; break;
		case SubEnv::max: fcOut = fcmax; break;
		case SubEnv::center: fcOut = fcmean; break;
		case SubEnv::userDefined: fcOut = m_UserDefinedFc; break;
	}

	GetOutputPort("output")->setCharacterizationFrequency(fcOut);
}
