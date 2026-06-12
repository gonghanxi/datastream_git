#include "IdentityCx_M_Block.h"

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

IdentityCx_M_Block::IdentityCx_M_Block(const std::string &name)
    :Block(name)
{

}

bool IdentityCx_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool IdentityCx_M_Block::Run()
{
    // 获取当前处理次数
    int i = m_IdentityCx_M->GetCount();

    SystemVueModelBuilder::DComplexMatrix outputMatrix;
    outputMatrix.Resize(m_RowsCols, m_RowsCols);

    // 实现 Identity_M 的原有逻辑
    if (i < m_InitialDelay)
    {
        // 在初始延迟期间输出零矩阵
        outputMatrix.Zero();
    }
    else
    {
        // 输出单位矩阵
        for (int m = 0; m < m_RowsCols; m++)
        {
            for (int n = 0; n < m_RowsCols; n++)
            {
                outputMatrix(m, n) = (m == n) ? 1 : 0;
            }
        }
    }

    // 创建输出数据向量并写入
    std::vector<SystemVueModelBuilder::DComplexMatrix> outputData;
    outputData.push_back(outputMatrix);
    WriteOutputData(GetOutputPortName(0), outputData);

    // 增加采样计数（对应 Advance()）
    m_IdentityCx_M->Advance();
    return true;
}

bool IdentityCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_IdentityCx_M = std::make_unique<IdentityCx_M>();

    SetDefaultParameters();

    try { m_RowsCols = std::stoi(getParameter("RowsCols").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RowsCols', using default value."); }
    try { m_ShowAdvancedParams = ConvertStringToSelectedShowAdvancedParams(getParameter("SelectedShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SelectedShowAdvancedParams', using default value."); }
    try { m_SampleRateOption = ConvertStringToSelectedSampleRateOption(getParameter("SelectedSampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SelectedSampleRateOption', using default value."); }
    try { m_InitialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }
    m_SampleRate = getSimu().samplingRate;

    SetParameters();

    if(!m_IdentityCx_M->Setup()) {
        return false;
    }

    AddOutputPort("output", m_IdentityCx_M->output,1, Block::DataType::MATRIX_TIME_DCOMPLEX);

    return true;
}

void IdentityCx_M_Block::SetParameters()
{
    if(!m_IdentityCx_M) return;
    m_IdentityCx_M->RowsCols = m_RowsCols;
    m_IdentityCx_M->ShowAdvancedParams = m_ShowAdvancedParams;
    m_IdentityCx_M->SampleRateOption = m_SampleRateOption;
    m_IdentityCx_M->SampleRate = m_SampleRate;
    m_IdentityCx_M->InitialDelay = m_InitialDelay;
}

IdentityCx_M::SelectedShowAdvancedParams IdentityCx_M_Block::ConvertStringToSelectedShowAdvancedParams(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return IdentityCx_M::No;
    }
    if (lower == "yes" || lower == "1") {
        return IdentityCx_M::Yes;
    }
    return IdentityCx_M::No;
}

IdentityCx_M::SelectedSampleRateOption IdentityCx_M_Block::ConvertStringToSelectedSampleRateOption(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return IdentityCx_M::UnTimed;
    }
    if (lower == "timedfromsamplerate" || lower == "1") {
        return IdentityCx_M::TimedFromSampleRate;
    }
    if (lower == "timedfromschematic" || lower == "2") {
        return IdentityCx_M::TimedFromSchematic;
    }
    return IdentityCx_M::UnTimed;
}

void IdentityCx_M_Block::SetDefaultParameters()
{
    m_RowsCols = 2;
    m_ShowAdvancedParams = IdentityCx_M::No;
    m_SampleRateOption = IdentityCx_M::TimedFromSchematic;
    m_SampleRate = getSimu().samplingRate;
    m_InitialDelay = 0;
}
