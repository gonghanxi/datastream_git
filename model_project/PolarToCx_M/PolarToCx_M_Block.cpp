#include "PolarToCx_M_Block.h"

#include <complex>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

PolarToCx_M_Block::PolarToCx_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool PolarToCx_M_Block::Setup()
{
    Block::Setup();

    m_magBuffer.clear();
    m_phaseBuffer.clear();
    m_outputQueue = std::queue<SystemVueModelBuilder::Matrix<std::complex<double>>>();

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool PolarToCx_M_Block::Run()
{
    if (IsVariableStepMode()) { return TimeDrivenRun(); }
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun：定步长单速率核心逻辑
// ============================================================================

bool PolarToCx_M_Block::DataStreamRun()
{
    // 读取 magnitude 矩阵
    auto magData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (magData.empty()) {
        return true;
    }

    // 读取 phase 矩阵
    auto phaseData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(1));
    if (phaseData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& magMx = magData[0];
    const SystemVueModelBuilder::DoubleMatrix& phaseMx = phaseData[0];

    const int NRow = magMx.NumRows();
    const int NCol = magMx.NumColumns();

    // 创建复数输出矩阵
    SystemVueModelBuilder::Matrix<std::complex<double>> outMx;
    outMx.Resize(NRow, NCol);

    for (int row = 0; row < NRow; ++row)
    {
        for (int col = 0; col < NCol; ++col)
        {
            outMx(row, col) = std::polar(magMx(row, col), phaseMx(row, col));
        }
    }

    std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputData;
    outputData.push_back(outMx);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun：变步长逐点处理 — magnitude + phase → 复数矩阵
// ============================================================================

bool PolarToCx_M_Block::TimeDrivenRun()
{
    // ① 累积 magnitude / phase 到 vector
    {
        auto magData   = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
        auto phaseData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(1));
        for (auto& v : magData)   m_magBuffer.push_back(v);
        for (auto& v : phaseData) m_phaseBuffer.push_back(v);
    }

    // ② 两个输入均非空 → 配对处理
    if (!m_magBuffer.empty() && !m_phaseBuffer.empty())
    {
        const SystemVueModelBuilder::DoubleMatrix& magMx   = m_magBuffer[0];
        const SystemVueModelBuilder::DoubleMatrix& phaseMx = m_phaseBuffer[0];

        const int NRow = magMx.NumRows();
        const int NCol = magMx.NumColumns();

        SystemVueModelBuilder::Matrix<std::complex<double>> outMx;
        outMx.Resize(NRow, NCol);

        for (int row = 0; row < NRow; ++row)
        {
            for (int col = 0; col < NCol; ++col)
            {
                outMx(row, col) = std::polar(magMx(row, col), phaseMx(row, col));
            }
        }

        m_outputQueue.push(outMx);
    }

    // ③ 出队写入，输出后清空输入 buffer
    if (!m_outputQueue.empty())
    {
        SystemVueModelBuilder::Matrix<std::complex<double>> out = m_outputQueue.front();
        m_outputQueue.pop();
        std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputVec;
        outputVec.push_back(out);
        WriteOutputData(GetOutputPortName(0), outputVec);

        m_magBuffer.clear();
        m_phaseBuffer.clear();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool PolarToCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_PolarToCx_M = std::make_unique<PolarToCx_M>();

    AddInputPort("magnitude", m_PolarToCx_M->magnitude, 1, Block::DataType::MATRIX_DOUBLE);
    AddInputPort("phase", m_PolarToCx_M->phase, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_PolarToCx_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}
