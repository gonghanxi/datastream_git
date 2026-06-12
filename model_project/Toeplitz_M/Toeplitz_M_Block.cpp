#include "Toeplitz_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

Toeplitz_M_Block::Toeplitz_M_Block(const std::string& name)
    : Block(name)
    , m_NumRows(2)
    , m_NumCols(2)
{
}

// ============================================================================
// Setup
// ============================================================================

bool Toeplitz_M_Block::Setup()
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
// Run
// ============================================================================

bool Toeplitz_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Toeplitz_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));

    const int expectedSize = m_NumRows + m_NumCols - 1;
    if (static_cast<int>(inputData.size()) < expectedSize) {
        return true;
    }

    SystemVueModelBuilder::DoubleMatrix outMat;
    outMat.Resize(m_NumRows, m_NumCols);

    for (int m = 0; m < m_NumRows; ++m)
    {
        for (int n = 0; n < m_NumCols; ++n)
        {
            outMat(m, n) = inputData[static_cast<size_t>(m_NumCols - n + m - 1)];
        }
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool Toeplitz_M_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);

    if (inputData.empty()) return true;

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    const int expectedSize = m_NumRows + m_NumCols - 1;

    if (static_cast<int>(m_inputBuffer.size()) >= expectedSize)
    {
        SystemVueModelBuilder::DoubleMatrix outMat;
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
        SystemVueModelBuilder::DoubleMatrix outValue = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
        outputData.push_back(outValue);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool Toeplitz_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Toeplitz_M = std::make_unique<Toeplitz_M>();

    SetDefaultParameters();

    try { m_NumRows = std::stoi(getParameter("NumRows").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumRows', using default value."); }
    try { m_NumCols = std::stoi(getParameter("NumCols").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumCols', using default value."); }

    SetParameters();

    AddInputPort("input", m_Toeplitz_M->input, static_cast<size_t>(m_NumRows + m_NumCols - 1), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Toeplitz_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void Toeplitz_M_Block::SetDefaultParameters()
{
    m_NumRows = 2;
    m_NumCols = 2;
}

void Toeplitz_M_Block::SetParameters()
{
    if (!m_Toeplitz_M) return;
    m_Toeplitz_M->NumRows = m_NumRows;
    m_Toeplitz_M->NumCols = m_NumCols;
}
