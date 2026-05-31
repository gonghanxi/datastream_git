#include "UnpackBus_M_Block.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

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

using DoubleMatrix = SystemVueModelBuilder::DoubleMatrix;

// ============================================================================
// 构造函数
// ============================================================================

UnpackBus_M_Block::UnpackBus_M_Block(const std::string& name)
    : Block(name)
    , m_NumRows(1)
    , m_NumCols(1)
    , m_Format(UnpackBus_M::ColumnMajor)
{
}

// ============================================================================
// Setup
// ============================================================================

bool UnpackBus_M_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool UnpackBus_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式：读入矩阵，一次性解包到所有 bus 通道
// ============================================================================

bool UnpackBus_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const DoubleMatrix& inMat = inputData[0];
    const int InRows = inMat.NumRows();
    const int InCols = inMat.NumColumns();

    std::vector<double> outputData;
    outputData.reserve(static_cast<size_t>(m_NumRows * m_NumCols));

    for (int m = 0; m < m_NumRows; ++m)
    {
        for (int n = 0; n < m_NumCols; ++n)
        {
            double val = (m < InRows && n < InCols) ? inMat(m, n) : 0.0;
            outputData.push_back(val);
        }
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点模式：逐帧输出一个 bus 通道
// ============================================================================

bool UnpackBus_M_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<DoubleMatrix> inputData = ReadInputData<DoubleMatrix>(inputPort);

    if (inputData.empty()) return true;

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= 1)
    {
        const DoubleMatrix& inMat = m_inputBuffer[0];
        const int InRows = inMat.NumRows();
        const int InCols = inMat.NumColumns();

        for (int m = 0; m < m_NumRows; ++m)
        {
            for (int n = 0; n < m_NumCols; ++n)
            {
                double val = (m < InRows && n < InCols) ? inMat(m, n) : 0.0;
                m_outputQueue.push(val);
            }
        }

        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        double val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<double> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool UnpackBus_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_UnpackBus_M = std::make_unique<UnpackBus_M>();

    SetDefaultParameters();

    try { m_NumRows = std::stoi(getParameter("NumRows").Value); } catch (...) {}
    try { m_NumCols = std::stoi(getParameter("NumCols").Value); } catch (...) {}
    try { m_Format   = ConvertStringToFormat(getParameter("Format").Value); } catch (...) {}

    SetParameters();

    if (m_NumRows < 1 || m_NumCols < 1)
    {
        LOG_ERROR("NumRows and NumCols must be >= 1.");
        return false;
    }

    AddInputPort("input", m_UnpackBus_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_UnpackBus_M->output, static_cast<size_t>(m_NumRows * m_NumCols), Block::DataType::DOUBLE_BUS);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void UnpackBus_M_Block::SetDefaultParameters()
{
    m_NumRows = 1;
    m_NumCols = 1;
    m_Format  = UnpackBus_M::ColumnMajor;
}

void UnpackBus_M_Block::SetParameters()
{
    if (!m_UnpackBus_M) return;
    m_UnpackBus_M->NumRows = m_NumRows;
    m_UnpackBus_M->NumCols = m_NumCols;
    m_UnpackBus_M->Format  = m_Format;
}

// ============================================================================
// 枚举转换
// ============================================================================

UnpackBus_M::SelectedFormat UnpackBus_M_Block::ConvertStringToFormat(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "columnmajor" || lower == "0") {
        return UnpackBus_M::ColumnMajor;
    }
    if (lower == "rowmajor" || lower == "1") {
        return UnpackBus_M::RowMajor;
    }
    return UnpackBus_M::ColumnMajor;
}
