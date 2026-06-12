#include "DeMux_Block.h"

DeMux_Block::DeMux_Block(const std::string &name)
    :Block(name)
{

}

bool DeMux_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool DeMux_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool DeMux_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_DeMux = std::make_unique<DeMux>();

    SetDefaultParameters();

    try { m_BlockSize = std::stod(getParameter("BlockSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BlockSize', using default value."); }

    SetParameters();

    if(!m_DeMux->Setup()) return false;

    AddInputPort("input", m_DeMux->input, static_cast<int>(m_BlockSize), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("control", m_DeMux->control, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_DeMux->output, static_cast<int>(m_BlockSize), DataType::DOUBLE_BUS);

    return true;
}

void DeMux_Block::SetParameters()
{
    if(!m_DeMux) return;
    m_DeMux->BlockSize = m_BlockSize;
}

void DeMux_Block::SetDefaultParameters()
{
    m_BlockSize = 1;
}

bool DeMux_Block::DataStreamRun()
{
    std::string input = GetInputPortName(0);
    std::string control = GetInputPortName(1);
    std::string output = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(input);
    std::vector<int> controlData = ReadInputData<int>(control);

    Buffer* outputBuffer = GetOutputPort(output);
    size_t numChannels = outputBuffer->GetBusConnectionCount();

    // 检查输入数据
    if (inputData.size() != m_BlockSize) {
        LOG_ERROR("Input data size does not match BlockSize");
        return false;
    }

    // 检查 control 数据
    if (controlData.empty()) {
        LOG_ERROR("Control input is empty");
        return false;
    }

    // 先关闭所有通道的写入权限
    for(size_t i = 0; i < numChannels; i++) {
        outputBuffer->SetBusConnectionPermitWrite(i, false);
    }

    // 准备输出数据（所有输入数据）
    std::vector<double> outputData;
    outputData.reserve(m_BlockSize);
    for(int j = 0; j < m_BlockSize; j++) {
        outputData.push_back(inputData[j]);
    }

    // 处理 control 数据：决定哪些通道接收数据
    int selectedChannel = controlData[0];
    if(selectedChannel < 0 || selectedChannel >= static_cast<int>(numChannels)) {
        LOG_ERROR("The control input can only accept values in the range [0, N - 1], where N is the output size.");
        return false;
    }
    outputBuffer->SetBusConnectionPermitWrite(selectedChannel, true);
    // 写入输出
    outputBuffer->WriteDataToChannel(selectedChannel, outputData);

    return true;
}

bool DeMux_Block::TimeDrivenRun()
{
    std::string input = GetInputPortName(0);
    std::string control = GetInputPortName(1);
    std::string output = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(input);
    std::vector<int> controlData = ReadInputData<int>(control);

    if(inputData.empty() || controlData.empty()) return true;

    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    m_controlBuffer.push_back(controlData[0]);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_BlockSize)
            && m_controlBuffer.size() >= 1) {
        Buffer* outputBuffer = GetOutputPort(output);
        size_t numChannels = outputBuffer->GetBusConnectionCount();

        // 先关闭所有通道的写入权限
        for(size_t i = 0; i < numChannels; i++) {
            outputBuffer->SetBusConnectionPermitWrite(i, false);
        }

        // 准备输出数据（所有输入数据）
        std::vector<double> outputData;
        outputData.reserve(m_BlockSize);
        for(int j = 0; j < m_BlockSize; j++) {
            outputData.push_back(m_inputBuffer[j]);
        }

        // 处理 control 数据：决定哪些通道接收数据
        int selectedChannel = m_controlBuffer[0];
        if(selectedChannel < 0 || selectedChannel >= static_cast<int>(numChannels)) {
            LOG_ERROR("The control input can only accept values in the range [0, N - 1], where N is the output size.");
            return false;
        }
        outputBuffer->SetBusConnectionPermitWrite(selectedChannel, true);
        // 写入输出
        // 每次输出一个数据，不会影响通道接收（输入固定为1，每次读取1个数据，输出1个数据，当需要切换通道时，已经将此通道输出完毕）
        for (const auto& val : outputData)
        {
            m_outputQueue.push(val);
        }
        if (!m_outputQueue.empty())
        {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            outputBuffer->WriteDataToChannel(selectedChannel, std::vector<int>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[DeMux_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}
