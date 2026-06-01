#include "AvgSqrErr_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

AvgSqrErr_M_Block::AvgSqrErr_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool AvgSqrErr_M_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool AvgSqrErr_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool AvgSqrErr_M_Block::DataStreamRun()
{
    std::string input1Port = GetInputPortName(0);
    std::string input2Port = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto input1Data = ReadInputData<SystemVueModelBuilder::Matrix<double>>(input1Port);
    auto input2Data = ReadInputData<SystemVueModelBuilder::Matrix<double>>(input2Port);

    if (input1Data.empty() || input2Data.empty()) return true;

    double totalSSE = 0.0;

    for (int n = 0; n < m_NumInputsToAverage; ++n)
    {
        const Matrix<double>& A = input1Data[static_cast<size_t>(n)];
        const Matrix<double>& B = input2Data[static_cast<size_t>(n)];

        if (A.NumRows() != B.NumRows() ||
            A.NumColumns() != B.NumColumns())
        {
            LOG_ERROR("AvgSqrErr_M: input1 and input2 matrices must have identical sizes.");
            std::vector<double> outputData(1, 0.0);
            WriteOutputData(outputPort, outputData);
            return false;
        }

        const size_t numElements = A.NumElements();
        double pairSSE = 0.0;

        for (size_t i = 0; i < numElements; ++i)
        {
            const double d = A(i) - B(i);
            pairSSE += d * d;
        }

        totalSSE += pairSSE;
    }

    std::vector<double> outputData(1, totalSSE / static_cast<double>(m_NumInputsToAverage));
    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool AvgSqrErr_M_Block::TimeDrivenRun()
{
    std::string input1Port = GetInputPortName(0);
    std::string input2Port = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto input1Data = ReadInputData<SystemVueModelBuilder::Matrix<double>>(input1Port);
    auto input2Data = ReadInputData<SystemVueModelBuilder::Matrix<double>>(input2Port);

    for (size_t i = 0; i < input1Data.size(); ++i)
        m_input1Buffer.push_back(input1Data[i]);
    for (size_t i = 0; i < input2Data.size(); ++i)
        m_input2Buffer.push_back(input2Data[i]);

    if (static_cast<int>(m_input1Buffer.size()) >= m_NumInputsToAverage
        && static_cast<int>(m_input2Buffer.size()) >= m_NumInputsToAverage)
    {
        double totalSSE = 0.0;

        for (int n = 0; n < m_NumInputsToAverage; ++n)
        {
            const Matrix<double>& A = m_input1Buffer[static_cast<size_t>(n)];
            const Matrix<double>& B = m_input2Buffer[static_cast<size_t>(n)];

            if (A.NumRows() != B.NumRows() ||
                A.NumColumns() != B.NumColumns())
            {
                LOG_ERROR("AvgSqrErr_M: input1 and input2 matrices must have identical sizes.");
                return false;
            }

            const size_t numElements = A.NumElements();
            double pairSSE = 0.0;

            for (size_t i = 0; i < numElements; ++i)
            {
                const double d = A(i) - B(i);
                pairSSE += d * d;
            }

            totalSSE += pairSSE;
        }

        double avg = totalSSE / static_cast<double>(m_NumInputsToAverage);
        m_outputQueue.push(avg);

        m_input1Buffer.clear();
        m_input2Buffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        double val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<double> outputData(1, val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool AvgSqrErr_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_AvgSqrErr_M = std::make_unique<AvgSqrErr_M>();

    SetDefaultParameters();
    try { m_NumInputsToAverage = std::stoi(getParameter("NumInputsToAverage").Value); } catch (...) {}
    SetParameters();
    if (!m_AvgSqrErr_M->Setup()) return false;

    if (m_NumInputsToAverage < 1)
    {
        LOG_ERROR("AvgSqrErr_M: NumInputsToAverage must be >= 1.");
        return false;
    }

    AddInputPort("input1", m_AvgSqrErr_M->input1, static_cast<size_t>(m_NumInputsToAverage), Block::DataType::MATRIX_DOUBLE);
    AddInputPort("input2", m_AvgSqrErr_M->input2, static_cast<size_t>(m_NumInputsToAverage), Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_AvgSqrErr_M->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void AvgSqrErr_M_Block::SetDefaultParameters()
{
    m_NumInputsToAverage = 8;
}

void AvgSqrErr_M_Block::SetParameters()
{
    if (!m_AvgSqrErr_M) return;
    m_AvgSqrErr_M->NumInputsToAverage = m_NumInputsToAverage;
}
