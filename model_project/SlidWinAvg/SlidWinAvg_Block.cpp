#include "SlidWinAvg_Block.h"

SlidWinAvg_Block::SlidWinAvg_Block(const std::string &name)
    :Block(name)
{

}

bool SlidWinAvg_Block::Setup()
{
    currentIndex = 0;
    currentSum = 0.0;
    if (WindowSize > 0)
    {
        slideWindow.Resize(1, WindowSize);
        slideWindow.Zero();
    }
    else
    {
        LOG_ERROR("WindowSize must > 0");
        return false;
    }
    Block::Setup();
    return true;
}

bool SlidWinAvg_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_SlidWinAvg = std::make_unique<SlidWinAvg>();
    SetDefaultParameters();
    try { WindowSize = std::stoi(getParameter("WindowSize").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'WindowSize', using default value."); }
    SetParameters();
    AddInputPort("input", m_SlidWinAvg->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_SlidWinAvg->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

bool SlidWinAvg_Block::Run()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(1);
    currentSum += inputData[0] - slideWindow(currentIndex);	// 移动滑窗，即减去最旧的值并加上新的输入值，求新滑窗内的和
    slideWindow(currentIndex) = inputData[0];				// 滑窗最旧的值更新为新输入的值
    currentIndex = (currentIndex + 1) % WindowSize;		// 更新指针（索引）
    outputData[0] = currentSum / WindowSize;
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

void SlidWinAvg_Block::SetParameters()
{
    if(!m_SlidWinAvg) return;
    m_SlidWinAvg->WindowSize = WindowSize;
}

void SlidWinAvg_Block::SetDefaultParameters()
{
    WindowSize = 3;
}
