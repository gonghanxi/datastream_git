#include "Inttoreal_Block.h"

#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

IntToReal_Block::IntToReal_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// 参数同步到算法实例（本算法无参数）
// ============================================================================

void IntToReal_Block::SetParameters()
{
    // 本算法无参数，无需同步
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool IntToReal_Block::Setup()
{
    Block::Setup();
    return true;
}

bool IntToReal_Block::Run()
{
    return DataStreamRun();
}

bool IntToReal_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<IntToReal>();

    // ---- 注册端口 ----
    AddInputPort("input",  m_algo->input,  1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑（int 转 double）
// ============================================================================

bool IntToReal_Block::DataStreamRun()
{
    SetParameters();

    auto inputData = ReadInputData<int>(GetInputPortName(0));
    if (inputData.empty()) { return false; }

    // 类型转换：int -> double
    std::vector<double> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        outputData.push_back(static_cast<double>(inputData[i]));
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}
