#include "ZeroCross_Block.h"

ZeroCross_Block::ZeroCross_Block(const std::string& name)
    : Block(name), m_previousInput(0.0)
{
}

bool ZeroCross_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    SetIsZeroCrossType(true);  // 标识自身类型（不依赖虚函数）

    m_zeroCross = std::make_unique<ZeroCross>();

    AddInputPort("input", m_zeroCross->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_zeroCross->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

bool ZeroCross_Block::Setup()
{
    Block::Setup();
    m_previousInput = 0.0;
    SetZeroCrossTriggered(false);
    return true;
}

bool ZeroCross_Block::Run()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        if (IsVariableStepMode()) {
            return true;
        }
        return false;
    }

    // Detect sign change (zero crossing)
    bool isCross = (m_previousInput * inputData[0] < 0);
    qDebug()<<"m_previousInput"<<m_previousInput;
    qDebug()<<"inputData[0]"<<inputData[0];
    m_previousInput = inputData[0];

    qDebug()<<"isCross"<<isCross;
    if (isCross) {
        // Crossing detected: set flag, suppress output
        SetZeroCrossTriggered(true);
        // Do NOT write to output - downstream blocks will be skipped by scheduler
        // Don't clear output buffer here - ResetBuffer resets read/write pointers
        // which can cause issues when the buffer is shared with downstream blocks
    } else {
        // No crossing: clear flag, pass data through
        SetZeroCrossTriggered(false);
        WriteOutputData(outputPortName, inputData);
    }

    return true;
}
