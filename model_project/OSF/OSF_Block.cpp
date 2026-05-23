#include "OSF_Block.h"
#include <sstream>
#include <algorithm>

OSF_Block::OSF_Block(const std::string& name)
    : Block(name)
{
}

bool OSF_Block::Setup()
{
    Block::Setup();
    return true;
}

bool OSF_Block::ValidateParameters()
{
    if (m_n < 1) {
        LOG_ERROR("N must be > 0.");
        return false;
    }

    if (m_percentile < 0) {
        LOG_WARN("Percentile < 0, clamped to 0.");
        m_percentile = 0;
    } else if (m_percentile > 100) {
        LOG_WARN("Percentile > 100, clamped to 100.");
        m_percentile = 100;
    }

    return true;
}

bool OSF_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    // 读取输入数据
    std::string inputPortName = GetInputPortName(0);
    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;  // 没有数据，等待下次调用
    }

    // 获取输入值（每次处理一个样本）
    double inputValue = inputData[0];

    // 设置到原模型
    m_OSF->m_input = inputValue;

    // 调用原模型的 Run 方法
    bool result = m_OSF->Run();

    if (result) {
        // 获取输出值
        double outputValue = m_OSF->m_output;

        // 写入输出
        Buffer* buffer = GetOutputPort(GetOutputPortName(0));
        buffer->WriteData(outputValue);
    }

    return result;
}

bool OSF_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_OSF = std::make_unique<SystemVueModelBuilder::OSF>();

    SetDefaultParameters();

    // 读取参数
    try {
        std::string value = getParameter("N").Value;
        if (!value.empty()) m_n = std::stoi(value);
    } catch (...) {}

    try {
        std::string value = getParameter("Percentile").Value;
        if (!value.empty()) m_percentile = std::stoi(value);
    } catch (...) {}

    // 验证参数
    if (!ValidateParameters()) {
        return false;
    }

    // 设置模型参数
    SetParameters();

    if(!m_OSF->Setup()) return false;

    // 添加端口
    AddInputPort("input",
                 m_OSF->m_input,
                 1,
                 Block::DataType::DOUBLE);

    AddOutputPort("output",
                  m_OSF->m_output,
                  1,
                  Block::DataType::DOUBLE);

    return true;
}

void OSF_Block::SetDefaultParameters()
{
    m_n = 3;
    m_percentile = 50;
}

void OSF_Block::SetParameters()
{
    if (!m_OSF) {
        return;
    }
    m_OSF->m_n = m_n;
    m_OSF->m_percentile = m_percentile;
}
