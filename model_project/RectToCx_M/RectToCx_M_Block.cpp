#include "RectToCx_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

RectToCx_M_Block::RectToCx_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool RectToCx_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RectToCx_M_Block::Run()
{
    return DataStreamRun();
}

bool RectToCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_RectToCx_M = std::make_unique<RectToCx_M>();

    // 注册端口（Matrix 数据）
    AddInputPort("real", m_RectToCx_M->real, 1, Block::DataType::MATRIX_DOUBLE);
    AddInputPort("imag", m_RectToCx_M->imag, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_RectToCx_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool RectToCx_M_Block::DataStreamRun()
{
    // 读取 real 和 imag 端口数据
    auto realData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(0));
    auto imagData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(1));

    // 获取矩阵维度
    size_t NRow = 1;
    size_t NCol = 1;

    bool hasReal = !realData.empty();
    bool hasImag = !imagData.empty();

    if (hasReal) {
        NRow = realData[0].NumRows();
        NCol = realData[0].NumColumns();
    } else if (hasImag) {
        NRow = imagData[0].NumRows();
        NCol = imagData[0].NumColumns();
    }

    // 创建输出矩阵
    SystemVueModelBuilder::Matrix<std::complex<double>> outputMatrix;
    outputMatrix.Resize(static_cast<int>(NRow), static_cast<int>(NCol));

    // 填充矩阵数据
    for (size_t row = 0; row < NRow; row++) {
        for (size_t col = 0; col < NCol; col++) {
            outputMatrix(row, col) = 0.0;

            if (hasReal && realData[0].NumRows() > row && realData[0].NumColumns() > col) {
                outputMatrix(row, col).real(realData[0](row, col));
            }

            if (hasImag && imagData[0].NumRows() > row && imagData[0].NumColumns() > col) {
                outputMatrix(row, col).imag(imagData[0](row, col));
            }
        }
    }

    // 写入输出
    std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputVec;
    outputVec.push_back(outputMatrix);
    WriteOutputData(GetOutputPortName(0), outputVec);

    return true;
}
