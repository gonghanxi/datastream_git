#include "Abs_M_Block.h"
#include <cmath>

Abs_M_Block::Abs_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

// Setup — 每轮仿真开始前初始化（Matrix 模型无状态，无需额外清理）
bool Abs_M_Block::Setup()
{
    Block::Setup();
    return true;
}

// Run — 仅支持数据流模式，直接分发到 DataStreamRun
bool Abs_M_Block::Run()
{
    return DataStreamRun();
}

// Initialize — 创建算法实例并注册 Matrix 输入/输出端口
bool Abs_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    // 创建算法实例
    m_Abs_M = std::make_unique<Abs_M>();

    // 注册端口：输入/输出均为 MATRIX_DOUBLE，速率 = 1（每次 firing 处理一个矩阵）
    AddInputPort("input", m_Abs_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Abs_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun — 核心运行逻辑：读取矩阵 → 逐元素取绝对值 → 输出
// ============================================================================

bool Abs_M_Block::DataStreamRun()
{
    // 读取输入矩阵（端口速率 = 1，每次取第一个元素即为当前矩阵）
    auto inputData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(0));
    if (inputData.empty()) return false;

    const SystemVueModelBuilder::Matrix<double>& inMat = inputData[0];

    // 创建输出矩阵，保持与输入相同的维度和尺寸
    SystemVueModelBuilder::Matrix<double> outMat;
    outMat.ResizeMultidimensional(inMat.NumDimensions(), inMat.Dimensions());

    // 逐元素计算绝对值：对每个位置 vi = |vi|
    const size_t n = inMat.NumElements();
    for (size_t i = 0; i < n; ++i) {
        double v = inMat(i);
        if (v < 0.0) v = -v;
        outMat(i) = v;
    }

    // 写入输出矩阵
    std::vector<SystemVueModelBuilder::Matrix<double>> outputVec;
    outputVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputVec);

    return true;
}
