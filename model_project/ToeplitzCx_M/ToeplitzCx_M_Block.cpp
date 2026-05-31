#include "ToeplitzCx_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

ToeplitzCx_M_Block::ToeplitzCx_M_Block(const std::string& name)
    : Block(name)
    , m_NumRows(2)
    , m_NumCols(2)
{
}

// ============================================================================
// Setup
// ============================================================================

bool ToeplitzCx_M_Block::Setup()
{
    Block::Setup();

    if (m_NumRows <= 0 || m_NumCols <= 0)
    {
        LOG_ERROR("NumRows and NumCols must be greater than 0.");
        return false;
    }

    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool ToeplitzCx_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool ToeplitzCx_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));

    const int expectedSize = m_NumRows + m_NumCols - 1;
    if (static_cast<int>(inputData.size()) < expectedSize) {
        return true;
    }

    SystemVueModelBuilder::DComplexMatrix outMat;
    outMat.Resize(m_NumRows, m_NumCols);

    for (int m = 0; m < m_NumRows; ++m)
    {
        for (int n = 0; n < m_NumCols; ++n)
        {
            outMat(m, n) = inputData[static_cast<size_t>(m_NumCols - n + m - 1)];
        }
    }

    std::vector<SystemVueModelBuilder::DComplexMatrix> outputData;
    outputData.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool ToeplitzCx_M_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPort);

    if (inputData.empty()) return true;

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    const int expectedSize = m_NumRows + m_NumCols - 1;

    if (static_cast<int>(m_inputBuffer.size()) >= expectedSize)
    {
        SystemVueModelBuilder::DComplexMatrix outMat;
        outMat.Resize(m_NumRows, m_NumCols);

        for (int m = 0; m < m_NumRows; ++m)
        {
            for (int n = 0; n < m_NumCols; ++n)
            {
                outMat(m, n) = m_inputBuffer[static_cast<size_t>(m_NumCols - n + m - 1)];
            }
        }

        m_outputQueue.push(outMat);
        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        SystemVueModelBuilder::DComplexMatrix outValue = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<SystemVueModelBuilder::DComplexMatrix> outputData;
        outputData.push_back(outValue);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool ToeplitzCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_ToeplitzCx_M = std::make_unique<ToeplitzCx_M>();

    SetDefaultParameters();

    try { m_NumRows = std::stoi(getParameter("NumRows").Value); } catch (...) {}
    try { m_NumCols = std::stoi(getParameter("NumCols").Value); } catch (...) {}

    SetParameters();

    AddInputPort("input", m_ToeplitzCx_M->input, static_cast<size_t>(m_NumRows + m_NumCols - 1), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_ToeplitzCx_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void ToeplitzCx_M_Block::SetDefaultParameters()
{
    m_NumRows = 2;
    m_NumCols = 2;
}

void ToeplitzCx_M_Block::SetParameters()
{
    if (!m_ToeplitzCx_M) return;
    m_ToeplitzCx_M->NumRows = m_NumRows;
    m_ToeplitzCx_M->NumCols = m_NumCols;
}
