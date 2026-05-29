#include "Abs_M_Block.h"
#include <cmath>

Abs_M_Block::Abs_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool Abs_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Abs_M_Block::Run()
{
    return DataStreamRun();
}

bool Abs_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Abs_M = std::make_unique<Abs_M>();

    // 注册端口（Matrix 数据）
    AddInputPort("input", m_Abs_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Abs_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool Abs_M_Block::DataStreamRun()
{
    // 读取输入矩阵
    auto inputData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(0));
    if (inputData.empty()) return false;

    const SystemVueModelBuilder::Matrix<double>& inMat = inputData[0];

    // 创建输出矩阵，保持相同维度
    SystemVueModelBuilder::Matrix<double> outMat;
    outMat.ResizeMultidimensional(inMat.NumDimensions(), inMat.Dimensions());

    // 逐元素计算绝对值
    const size_t n = inMat.NumElements();
    for (size_t i = 0; i < n; ++i) {
        double v = inMat(i);
        if (v < 0.0) v = -v;
        outMat(i) = v;
    }

    // 写入输出
    std::vector<SystemVueModelBuilder::Matrix<double>> outputVec;
    outputVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputVec);

    return true;
}
