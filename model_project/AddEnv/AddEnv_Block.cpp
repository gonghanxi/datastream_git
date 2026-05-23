#include "AddEnv_Block.h"
#include "TimedDFModel.h"

AddEnv_Block::AddEnv_Block(const std::string& name)
    : Block(name)
{
}

bool AddEnv_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

bool AddEnv_Block::Run()
{
    //时间驱动
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool AddEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_addEnv = std::make_unique<AddEnv>();

    AddInputPort("input", m_addEnv->input, 1, DataType::ENVELOPE_BUS);
    AddOutputPort("output", m_addEnv->output, 1, DataType::ENVELOPE_SIGNAL);

    SetDefaultParameters();

    simulator_param = getSimu();

    m_OutputFc = ConvertStringToSelectedOutputFc(getParameter("OutputFc").Value);
    m_UserDefinedFc = std::stod(getParameter("UserDefinedFc").Value);

    SetParameters(m_UserDefinedFc, m_OutputFc);

    return true;
}

void AddEnv_Block::SetParameters(double UserDefinedFc, AddEnv::SelectedOutputFc OutputFc)
{
    if(m_addEnv) {
        m_addEnv->UserDefinedFc = UserDefinedFc;
        m_addEnv->OutputFc = OutputFc;
        m_addEnv->output.SetSampleRate(simulator_param.samplingRate);
    }
}

AddEnv::SelectedOutputFc AddEnv_Block::ConvertStringToSelectedOutputFc(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "min" || lowerValue == "0") {
        return AddEnv::min;
    } else if (lowerValue == "max" || lowerValue == "1") {
        return AddEnv::max;
    } else if (lowerValue == "center" || lowerValue == "2") {
        return AddEnv::center;
    } else if (lowerValue == "userdefined" || lowerValue == "3") {
        return AddEnv::userDefined;
    }
    return AddEnv::center;
}

void AddEnv_Block::SetDefaultParameters()
{
    m_UserDefinedFc = 100e6;
    m_OutputFc = AddEnv::center;

}

void AddEnv_Block::PropagateCharacterizationFrequency()
{

    fcmax = 0.0;
    fcmean = 0.0;
    fcmin = GetInputPort(GetInputPortName(0))->GetBusConnections().begin()->bridgeReader->getCharacterizationFrequency(); //取其中一个通道的特征频率对最小值进行初始化


    // 求出各通道输入的最大特征频率、最小特征频率以及平均特征频率

    int	ChannelNumIn = GetInputPort(GetInputPortName(0))->GetBusConnectionCount();
    for (int i = 0; i < ChannelNumIn; i++)
    {
        fc = GetInputPort(GetInputPortName(0))->GetBusConnections().at(i).bridgeReader->getCharacterizationFrequency();

        fcmax = (fcmax < fc ? fc : fcmax);

        fcmin = (fcmin > fc ? fc : fcmin);

        fcmean += fc; // 每个循环累计一个通道内的载频
    }
    fcmean /= ChannelNumIn; // 求平均载频

    // 统一化载频

    switch (m_OutputFc)
    {
        case AddEnv::min:
        {
            fcOut = fcmin;
            break;
        }
        case AddEnv::max:
        {
            fcOut = fcmax;
            break;
        }
        case AddEnv::center:
        {
            fcOut = fcmean;
            break;
        }
        case AddEnv::userDefined:
        {
            fcOut = m_UserDefinedFc;
            break;
        }
    }

    GetOutputPort("output")->setCharacterizationFrequency(fcOut);

}

bool AddEnv_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::vector<SystemVueModelBuilder::EnvelopeSignal> inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);


    if (inputData.empty()) {
        std::cout << "ERROR: No input data available" << std::endl;
        return false;
    }

    PropagateCharacterizationFrequency();
    double outputFc = fcOut; // 输出载频

    const double dTime = (simulator_param.samplingRate > 0.0)
        ? (simulator_param.startTime + static_cast<double>(m_addEnv->GetCount()) / simulator_param.samplingRate)
        : 0.0;

    std::complex<double> acc(0.0, 0.0);
    for (size_t i = 0; i < inputData.size(); ++i) {
        double inputFc = GetOutputPort("output")->getCharacterizationFrequency();
        std::complex<double> converted = inputData[i].complex();
        if (inputFc != outputFc) {
            double phase = 2.0 * M_PI * (inputFc - outputFc) * dTime;
            converted *= std::complex<double>(cos(phase), sin(phase));
        }
        acc += converted;
    }

    // 步骤5：写入输出
    std::string outputPortName = GetOutputPortName(0);
    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.push_back(acc);
    WriteOutputData(outputPortName, outputData);

    GetOutputPort(outputPortName)->setCharacterizationFrequency(fcOut);
    m_addEnv->Advance();

    return true;
}

bool AddEnv_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    //保证多输入同时读取数据
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
            EnvelopeSignal sum;

            // 遍历每个缓冲区，累加第i个元素
            for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                sum += it->second[i];
            }

            outputData[i] = sum;
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

            qDebug() << "[AddEnv_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}
