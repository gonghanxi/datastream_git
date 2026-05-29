#include "CxToRect_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

CxToRect_M_Block::CxToRect_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool CxToRect_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool CxToRect_M_Block::Run()
{
    return DataStreamRun();
}

bool CxToRect_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_CxToRect_M = std::make_unique<CxToRect_M>();

    // 注册端口（Matrix 数据）
    AddInputPort("input", m_CxToRect_M->input, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("real", m_CxToRect_M->real, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("imag", m_CxToRect_M->imag, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool CxToRect_M_Block::DataStreamRun()
{
    // 读取输入复数矩阵
    auto inputData = ReadInputData<SystemVueModelBuilder::Matrix<std::complex<double>>>(GetInputPortName(0));
    if (inputData.empty()) return false;

    // 获取矩阵维度
    size_t NRow = inputData[0].NumRows();
    size_t NCol = inputData[0].NumColumns();

    // 创建输出矩阵
    SystemVueModelBuilder::Matrix<double> realMatrix;
    SystemVueModelBuilder::Matrix<double> imagMatrix;
    realMatrix.Resize(static_cast<int>(NRow), static_cast<int>(NCol));
    imagMatrix.Resize(static_cast<int>(NRow), static_cast<int>(NCol));

    // 提取实部和虚部
    for (size_t row = 0; row < NRow; row++) {
        for (size_t col = 0; col < NCol; col++) {
            realMatrix(row, col) = inputData[0](row, col).real();
            imagMatrix(row, col) = inputData[0](row, col).imag();
        }
    }

    // 写入输出
    std::vector<SystemVueModelBuilder::Matrix<double>> realVec;
    realVec.push_back(realMatrix);
    WriteOutputData(GetOutputPortName(0), realVec);

    std::vector<SystemVueModelBuilder::Matrix<double>> imagVec;
    imagVec.push_back(imagMatrix);
    WriteOutputData(GetOutputPortName(1), imagVec);

    return true;
}
