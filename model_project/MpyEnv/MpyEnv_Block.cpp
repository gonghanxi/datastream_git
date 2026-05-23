#include "MpyEnv_Block.h"
#include <algorithm>
#include <complex>
#include <cmath>

MpyEnv_Block::MpyEnv_Block(const std::string& name)
	: Block(name)
{
}

bool MpyEnv_Block::Setup()
{
	Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
	return true;
}

bool MpyEnv_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    PropagateCharacterizationFrequency();
    double outputFc = fcOut;

    const double dTime = (simulator_param.samplingRate > 0.0)
        ? (simulator_param.startTime + static_cast<double>(m_mpyEnv->GetCount()) / simulator_param.samplingRate)
        : 0.0;

    using SystemVueModelBuilder::EnvelopeSignal;
    std::complex<double> prod(1.0, 0.0);
    for (size_t i = 0; i < inputData.size(); ++i) {
        double inputFc = GetInputPort(inputPortName)
            ->GetBusConnections()
            .at(static_cast<int>(i))
            .bridgeReader->getCharacterizationFrequency();

        EnvelopeSignal converted(inputData[i]);
        EnvelopeSignal transformedSignal = converted.ConvertToNewFc(inputFc, outputFc, dTime);
        prod *= transformedSignal.complex();
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.push_back(prod);
    WriteOutputData(outputPortName, outputData);
    m_mpyEnv->Advance();
    GetOutputPort(outputPortName)->setCharacterizationFrequency(fcOut);

    return true;
}

bool MpyEnv_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();

    for(const auto& bridge_reader : bridge_readers) {

        std::vector<EnvelopeSignal> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if(inputData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < inputData.size();i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    bool CanProcessData = true;
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() >= 1) {
            CanProcessData = false;
            break;
        }
    }

    std::vector<EnvelopeSignal> outputData(1);  // 初始化为0

    if(CanProcessData) {
        // 遍历每个位置
        for(size_t i = 0; i < 1; ++i) {
            std::complex<double> acc(1.0,0.0);


            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                EnvelopeSignal converted(it->second[i]);
                acc *= converted.complex();
            }

            outputData[i] = acc;
            //将处理结果放入输出队列
            m_outputQueue.push(outputData[i]);
        }
        PropagateCharacterizationFrequency();
        //执行写入
        if (!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<EnvelopeSignal>{outputValue});
            GetOutputPort(outputPort)->setCharacterizationFrequency(fcOut);
            m_lastOutput = outputValue;

            qDebug() << "[MpyEnv_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

bool MpyEnv_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool MpyEnv_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_mpyEnv = std::make_unique<MpyEnv>();

    AddInputPort("input", m_mpyEnv->input, 1, Block::DataType::ENVELOPE_BUS);
	AddOutputPort("output", m_mpyEnv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

	SetDefaultParameters();

	simulator_param = getSimu();

	try { m_FcOut = ConvertStringToSelectedFcOut(getParameter("FcOut").Value); } catch (...) { }
	try { m_UserDefinedFc = std::stod(getParameter("UserDefinedFc").Value); } catch (...) { }

    SetParameters(m_UserDefinedFc, m_FcOut);
	return true;
}

void MpyEnv_Block::SetDefaultParameters()
{
	m_UserDefinedFc = 100e6;
	m_FcOut = MpyEnv::center;
}

void MpyEnv_Block::SetParameters(double userDefinedFc, MpyEnv::SelectedFcOut fcOut)
{
	if (m_mpyEnv) {
		m_mpyEnv->UserDefinedFc = userDefinedFc;
		m_mpyEnv->FcOut = fcOut;
		m_mpyEnv->output.SetSampleRate(simulator_param.samplingRate);
	}
}

MpyEnv::SelectedFcOut MpyEnv_Block::ConvertStringToSelectedFcOut(const std::string& value)
{
	std::string trimmedValue = value;
	trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
	trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

	if (lowerValue == "min" || lowerValue == "0") {
		return MpyEnv::min;
	} else if (lowerValue == "max" || lowerValue == "1") {
		return MpyEnv::max;
	} else if (lowerValue == "center" || lowerValue == "2") {
		return MpyEnv::center;
	} else if (lowerValue == "userdefined" || lowerValue == "user defined" || lowerValue == "3") {
		return MpyEnv::userDefined;
	}

	return MpyEnv::center;
}

void MpyEnv_Block::PropagateCharacterizationFrequency()
{
    int channelNumIn = GetInputPort(GetInputPortName(0))->GetBusConnectionCount();
	if (channelNumIn <= 0) {
		fcOut = 0.0;
		GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fcOut);
		return;
	}

	fcmax = 0.0;
	fcmean = 0.0;
    fcmin = GetInputPort(GetInputPortName(0))
		->GetBusConnections()
		.begin()->bridgeReader->getCharacterizationFrequency();

	for (int i = 0; i < channelNumIn; ++i) {
		fc = GetInputPort(GetInputPortName(0))
			->GetBusConnections()
			.at(i).bridgeReader->getCharacterizationFrequency();
		fcmax = (fcmax < fc ? fc : fcmax);
		fcmin = (fcmin > fc ? fc : fcmin);
		fcmean += fc;
	}
	fcmean /= channelNumIn;

	switch (m_FcOut)
	{
	case MpyEnv::min:
		fcOut = fcmin;
		break;
	case MpyEnv::max:
		fcOut = fcmax;
		break;
	case MpyEnv::center:
		fcOut = fcmean;
		break;
	case MpyEnv::userDefined:
		fcOut = m_UserDefinedFc;
		break;
	default:
		fcOut = fcmean;
		break;
	}

    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fcOut);
}




