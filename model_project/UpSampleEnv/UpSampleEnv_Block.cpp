#include "UpSampleEnv_Block.h"

//template class UPSAMPLEENV_API std::map<std::string, Parameter>;
//template class UPSAMPLEENV_API std::map<int, PortMsg>;

UpSampleEnv_Block::UpSampleEnv_Block(const std::string &name)
    : Block(name)
{

}

bool UpSampleEnv_Block::Setup()
{
    Block::Setup();
    return true;
}

bool UpSampleEnv_Block::Run()
{
     if(!CanProcess()) {
         return false;
     }

    // 获取输入输出端口名称
    BufferReader* inputport = GetInputPort(GetInputPortName(0));
    std::string outputPortName = GetOutputPortName(0);


    std::vector<SystemVueModelBuilder::EnvelopeSignal> inputData;
    inputData.reserve(inputport->GetReadSize());
    inputport->ReadData(inputData);
    inputport->getCharacterizationFrequency();
    if(inputData.empty()) {
        return false;
    }

    //----------------数据处理---------------------
    size_t inLen = 0;
    for(size_t i = 0; i < inputport->GetReadSize(); i++) {
        if(inputData[i] == 0) {
            break;
        }
        inLen++;
    }
    size_t outLen = inLen * m_factor;
    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(outLen);
    if(m_mode == UpSampleEnv::Insertzeros) {
        for(size_t i = 0; i < inLen; i++) {
            SystemVueModelBuilder::EnvelopeSignal InputSample = inputData[i];
            outputData.push_back(InputSample);
            for(int j = 1; j < m_factor; j++) {
                outputData.push_back(0);
            }
        }
    }
    else if(m_mode == UpSampleEnv::Holdsample) {
        for(size_t i = 0; i < inLen; i++) {
            SystemVueModelBuilder::EnvelopeSignal InputSample = inputData[i];
            for(int j = 0; j < m_factor; j++) {
                outputData.push_back(InputSample);
            }
        }
    }
    else if(m_mode == UpSampleEnv::Linear) {
        // 线性插值模式
        for(size_t i = 0; i < inLen; i++) {
            SystemVueModelBuilder::EnvelopeSignal currentSample = inputData[i];

            if(i == 0) {
                // 处理第一个输入点
                // 第一个输出点设为0
                outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(std::complex<double>(0.0, 0.0)));

                // 在0和第一个输入点之间插值
                for(int j = 1; j < m_factor; j++) {
                    double t = static_cast<double>(j) / m_factor;
                    std::complex<double> interpValue = std::complex<double>(0.0, 0.0) +
                                                       t * (currentSample.complex() - std::complex<double>(0.0, 0.0));
                    outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(interpValue));
                }
            }
            else {
                // 处理后续输入点
                SystemVueModelBuilder::EnvelopeSignal prevSample = inputData[i-1];

                // 在前一个点和当前点之间插值
                for(int j = 0; j < m_factor; j++) {
                    double t = static_cast<double>(j) / m_factor;
                    std::complex<double> interpValue = prevSample.complex() +
                                                       t * (currentSample.complex() - prevSample.complex());
                    outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(interpValue));
                }
            }
        }
    }

    //----------------数据处理---------------------

    //----------------写入数据---------------------
    WriteOutputData(outputPortName, outputData);

    UpdateCharacterizationFrequency();
    Buffer* buffer = GetOutputPort(outputPortName);
    buffer->getCharacterizationFrequency();
    return true;
}

bool UpSampleEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_upsampleEnv = std::make_unique<UpSampleEnv>();

    AddInputPort("input", m_upsampleEnv->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_upsampleEnv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParameters();

    m_factor = std::stoi(getParameter("Factor").Value);

    //*******************待修改
    m_phase = std::stoi(getParameter("Phase").Value);

    m_mode = ConvertStringToModeEnum(getParameter("Mode").Value);

    SetParameters(m_factor, m_phase, m_mode);

    //输出端的速率改变
    GetOutputPort(GetOutputPortName(0))->SetWriteSize(m_factor);
    //输入端的速率改变
    m_upsampleEnv->Initialize();
    return true;
}

void UpSampleEnv_Block::SetDefaultParameters()
{
    m_factor = 5;
    m_phase = 0;
    m_mode = UpSampleEnv::Holdsample;
}

void UpSampleEnv_Block::SetParameters(int factor, int phase, UpSampleEnv::ModeEnum mode)
{
    m_factor = factor;
    m_phase = phase;
    m_mode = mode;
    if(m_upsampleEnv) {
        m_upsampleEnv->Factor = m_factor;
        m_upsampleEnv->Phase = m_phase;
        m_upsampleEnv->Mode = m_mode;
    }
}

void UpSampleEnv_Block::UpdateCharacterizationFrequency()
{
    if(m_upsampleEnv->input.IsConnected()) {
        m_upsampleEnv->PropagateCharacterizationFrequency();
    }
}

UpSampleEnv::ModeEnum UpSampleEnv_Block::ConvertStringToModeEnum(const std::string &value)
{
    // 去除所有空格（包括前后和中间的空格）
    std::string trimmedValue;
    trimmedValue.reserve(value.size()); // 预分配空间以提高效率

    // 遍历原字符串，只保留非空白字符
    for (char c : value) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            trimmedValue.push_back(c);
        }
    }
    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 字符串匹配
    if (lowerValue == "insertzeros" || lowerValue == "0") {
        return UpSampleEnv::Insertzeros;
    } else if (lowerValue == "holdsample" || lowerValue == "1") {
        return UpSampleEnv::Holdsample;
    } else if (lowerValue == "polyphasefilter" || lowerValue == "2") {
        return UpSampleEnv::Polyphasefilter;
    } else if (lowerValue == "linear" || lowerValue == "3") {
        return UpSampleEnv::Linear;
    } else {
        return UpSampleEnv::Holdsample;
    }
}
