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
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool PolarToCx_M_Block::Run()
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
