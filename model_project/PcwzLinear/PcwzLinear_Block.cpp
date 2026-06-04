#include "PcwzLinear_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

PcwzLinear_Block::PcwzLinear_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool PcwzLinear_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool PcwzLinear_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool PcwzLinear_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) return true;

    const double x = inputData[0];
    double y = m_breakpointsY.back(); // 默认用最后一个端点 y 值

    for (int i = 0; i < m_numBreakpoints; ++i)
    {
        const double xi = m_breakpointsX[i];
        if (x < xi)
        {
            // i==0 时，等于第一个端点 y；i>0 时用 slope(i)*x + intercept(i)
            y = (i > 0) ? (m_slope[i - 1] * x + m_intercept[i - 1]) : m_breakpointsY[0];
            break;
        }
    }

    std::vector<double> outputData;
    outputData.push_back(y);
    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool PcwzLinear_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= 1)
    {
        const double x = m_inputBuffer[0];
        double y = m_breakpointsY.back();

        for (int i = 0; i < m_numBreakpoints; ++i)
        {
            const double xi = m_breakpointsX[i];
            if (x < xi)
            {
                y = (i > 0) ? (m_slope[i - 1] * x + m_intercept[i - 1]) : m_breakpointsY[0];
                break;
            }
        }

        m_outputQueue.push(y);
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

bool PcwzLinear_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_PcwzLinear = std::make_unique<PcwzLinear>();

    // 解析 Breakpoints 参数（复数矩阵）
    try { m_Breakpoints = ParseStringToMatrix<std::complex<double>>(getParameter("numBreakpoints").Value); } catch (...) {}

    m_numBreakpoints = static_cast<int>(m_Breakpoints.NumElements());
    if (m_numBreakpoints < 2)
    {
        LOG_ERROR("PcwzLinear: numBreakpoints must be >= 2.");
        return false;
    }

    // 从参数提取 x/y 坐标
    m_breakpointsX.clear();
    m_breakpointsY.clear();
    for (int i = 0; i < m_numBreakpoints; ++i)
    {
        m_breakpointsX.push_back(m_Breakpoints(i).real());
        m_breakpointsY.push_back(m_Breakpoints(i).imag());
    }

    // 校验 x 单调递增并预计算斜率/截距
    m_slope.resize(m_numBreakpoints - 1);
    m_intercept.resize(m_numBreakpoints - 1);
    for (int i = 1; i < m_numBreakpoints; ++i)
    {
        const double x1 = m_breakpointsX[i - 1];
        const double y1 = m_breakpointsY[i - 1];
        const double x2 = m_breakpointsX[i];
        const double y2 = m_breakpointsY[i];

        if (x1 >= x2)
        {
            LOG_ERROR("PcwzLinear: Breakpoints x values must be monotonically increasing.");
            return false;
        }

        m_slope[i - 1]     = (y2 - y1) / (x2 - x1);
        m_intercept[i - 1] = (x2 * y1 - y2 * x1) / (x2 - x1);
    }

    AddInputPort("input",  m_PcwzLinear->input,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_PcwzLinear->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
    
}
