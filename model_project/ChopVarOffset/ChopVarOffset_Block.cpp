#include "ChopVarOffset_Block.h"

#include <algorithm>

// ============================================================================
// 构造函数
// ============================================================================

ChopVarOffset_Block::ChopVarOffset_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool ChopVarOffset_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool ChopVarOffset_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool ChopVarOffset_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string offsetPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData  = ReadInputData<double>(inputPort);
    auto offsetData = ReadInputData<int>(offsetPort);

    if (inputData.empty() || offsetData.empty()) return true;

    const int Offset = offsetData[0];

    // ComputeRange 逻辑
    size_t iReadFrom;
    size_t iWriteTo;

    if (Offset > 0)
    {
        iReadFrom = 0;
        iWriteTo  = static_cast<size_t>(Offset);
    }
    else
    {
        long long negK = -(static_cast<long long>(Offset));
        if (negK < 0) negK = 0;
        iReadFrom = static_cast<size_t>(negK);
        iWriteTo  = 0;
    }

    const size_t iReadBufSize  = static_cast<size_t>(m_nRead);
    const size_t iWriteBufSize = static_cast<size_t>(m_nWrite);

    size_t iWriteNum;
    size_t iZeroPadNum;

    if (iWriteTo >= iWriteBufSize)
    {
        iWriteNum   = 0;
        iWriteTo    = iWriteBufSize;
        iZeroPadNum = 0;
    }
    else
    {
        size_t availableFromInput =
            (iReadFrom < iReadBufSize) ? (iReadBufSize - iReadFrom) : 0;

        iWriteNum = (std::min)(iWriteBufSize - iWriteTo, availableFromInput);

        size_t iZeroPadFrom = iWriteTo + iWriteNum;
        iZeroPadNum = (iZeroPadFrom < iWriteBufSize) ? (iWriteBufSize - iZeroPadFrom) : 0;
    }

    // 构造输出
    std::vector<double> outputData;
    outputData.reserve(iWriteBufSize);

    // 前导零填充
    for (size_t i = 0; i < iWriteTo; ++i)
        outputData.push_back(0.0);

    // 拷贝有效数据
    for (size_t i = 0; i < iWriteNum; ++i)
        outputData.push_back(inputData[iReadFrom + i]);

    // 尾部零填充
    for (size_t i = 0; i < iZeroPadNum; ++i)
        outputData.push_back(0.0);

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool ChopVarOffset_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string offsetPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData  = ReadInputData<double>(inputPort);
    auto offsetData = ReadInputData<int>(offsetPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < offsetData.size(); ++i)
        m_offsetBuffer.push_back(offsetData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_nRead
        && static_cast<int>(m_offsetBuffer.size()) >= 1)
    {
        const int Offset = m_offsetBuffer[0];

        size_t iReadFrom;
        size_t iWriteTo;

        if (Offset > 0)
        {
            iReadFrom = 0;
            iWriteTo  = static_cast<size_t>(Offset);
        }
        else
        {
            long long negK = -(static_cast<long long>(Offset));
            if (negK < 0) negK = 0;
            iReadFrom = static_cast<size_t>(negK);
            iWriteTo  = 0;
        }

        const size_t iReadBufSize  = static_cast<size_t>(m_nRead);
        const size_t iWriteBufSize = static_cast<size_t>(m_nWrite);

        size_t iWriteNum;
        size_t iZeroPadNum;

        if (iWriteTo >= iWriteBufSize)
        {
            iWriteNum   = 0;
            iWriteTo    = iWriteBufSize;
            iZeroPadNum = 0;
        }
        else
        {
            size_t availableFromInput =
                (iReadFrom < iReadBufSize) ? (iReadBufSize - iReadFrom) : 0;

            iWriteNum = (std::min)(iWriteBufSize - iWriteTo, availableFromInput);

            size_t iZeroPadFrom = iWriteTo + iWriteNum;
            iZeroPadNum = (iZeroPadFrom < iWriteBufSize) ? (iWriteBufSize - iZeroPadFrom) : 0;
        }

        // 前导零
        for (size_t i = 0; i < iWriteTo; ++i)
            m_outputQueue.push(0.0);
        // 有效数据
        for (size_t i = 0; i < iWriteNum; ++i)
            m_outputQueue.push(m_inputBuffer[iReadFrom + i]);
        // 尾部零
        for (size_t i = 0; i < iZeroPadNum; ++i)
            m_outputQueue.push(0.0);

        m_inputBuffer.clear();
        m_offsetBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        double val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<double> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool ChopVarOffset_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_ChopVarOffset = std::make_unique<ChopVarOffset>();

    SetDefaultParameters();
    try { m_nRead  = std::stoi(getParameter("nRead").Value);  } catch(...) { LOG_WARN("Failed to parse parameter 'nRead', using default value."); }
    try { m_nWrite = std::stoi(getParameter("nWrite").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'nWrite', using default value."); }
    SetParameters();
    if (!m_ChopVarOffset->Setup()) return false;

    if (m_nRead <= 0)
    {
        LOG_ERROR("nRead must be > 0");
        return false;
    }
    if (m_nWrite <= 0)
    {
        LOG_ERROR("nWrite must be > 0");
        return false;
    }

    AddInputPort("input",       m_ChopVarOffset->input,       static_cast<size_t>(m_nRead),  Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("offsetCntrl", m_ChopVarOffset->offsetCntrl, 1,                             Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output",     m_ChopVarOffset->output,      static_cast<size_t>(m_nWrite), Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void ChopVarOffset_Block::SetDefaultParameters()
{
    m_nRead  = 128;
    m_nWrite = 64;
}

void ChopVarOffset_Block::SetParameters()
{
    if (!m_ChopVarOffset) return;
    m_ChopVarOffset->nRead  = m_nRead;
    m_ChopVarOffset->nWrite = m_nWrite;
}
