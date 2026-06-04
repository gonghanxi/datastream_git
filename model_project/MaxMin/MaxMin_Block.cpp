#include "MaxMin_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
} // namespace

// ============================================================================
// 构造函数
// ============================================================================

MaxMin_Block::MaxMin_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool MaxMin_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    while (!m_indexQueue.empty())  m_indexQueue.pop();
    m_inputBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool MaxMin_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool MaxMin_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);
    std::string indexPort  = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) return true;

    if (m_MaxOrMin == MaxMin::min)
    {
        int minIndex = 0;
        double minValue = inputData[0];
        double minMagnitude = std::abs(inputData[0]);

        for (int i = 1; i < m_N; ++i)
        {
            if (m_Compare == MaxMin::valueIn && inputData[i] < minValue)
            {
                minIndex = i;
                minValue = inputData[i];
            }
            if (m_Compare == MaxMin::magnitudeIn && std::abs(inputData[i]) < minMagnitude)
            {
                minIndex = i;
                minMagnitude = std::abs(inputData[i]);
            }
        }

        std::vector<int> indexData;
        indexData.push_back(minIndex);
        WriteOutputData(indexPort, indexData);

        std::vector<double> outputData;
        if (m_OutputType == MaxMin::valueOut)
            outputData.push_back(inputData[minIndex]);
        else
            outputData.push_back(std::abs(inputData[minIndex]));
        WriteOutputData(outputPort, outputData);
    }
    else // max
    {
        int maxIndex = 0;
        double maxValue = inputData[0];
        double maxMagnitude = std::abs(inputData[0]);

        for (int i = 1; i < m_N; ++i)
        {
            if (m_Compare == MaxMin::valueIn && inputData[i] > maxValue)
            {
                maxIndex = i;
                maxValue = inputData[i];
            }
            if (m_Compare == MaxMin::magnitudeIn && std::abs(inputData[i]) > maxMagnitude)
            {
                maxIndex = i;
                maxMagnitude = std::abs(inputData[i]);
            }
        }

        std::vector<int> indexData;
        indexData.push_back(maxIndex);
        WriteOutputData(indexPort, indexData);

        std::vector<double> outputData;
        if (m_OutputType == MaxMin::valueOut)
            outputData.push_back(inputData[maxIndex]);
        else
            outputData.push_back(std::abs(inputData[maxIndex]));
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool MaxMin_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);
    std::string indexPort  = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_N)
    {
        double result;
        int    idx;

        if (m_MaxOrMin == MaxMin::min)
        {
            idx = 0;
            double minValue = m_inputBuffer[0];
            double minMagnitude = std::abs(m_inputBuffer[0]);

            for (int i = 1; i < m_N; ++i)
            {
                if (m_Compare == MaxMin::valueIn && m_inputBuffer[i] < minValue)
                {
                    idx = i;
                    minValue = m_inputBuffer[i];
                }
                if (m_Compare == MaxMin::magnitudeIn && std::abs(m_inputBuffer[i]) < minMagnitude)
                {
                    idx = i;
                    minMagnitude = std::abs(m_inputBuffer[i]);
                }
            }

            if (m_OutputType == MaxMin::valueOut)
                result = m_inputBuffer[idx];
            else
                result = std::abs(m_inputBuffer[idx]);
        }
        else // max
        {
            idx = 0;
            double maxValue = m_inputBuffer[0];
            double maxMagnitude = std::abs(m_inputBuffer[0]);

            for (int i = 1; i < m_N; ++i)
            {
                if (m_Compare == MaxMin::valueIn && m_inputBuffer[i] > maxValue)
                {
                    idx = i;
                    maxValue = m_inputBuffer[i];
                }
                if (m_Compare == MaxMin::magnitudeIn && std::abs(m_inputBuffer[i]) > maxMagnitude)
                {
                    idx = i;
                    maxMagnitude = std::abs(m_inputBuffer[i]);
                }
            }

            if (m_OutputType == MaxMin::valueOut)
                result = m_inputBuffer[idx];
            else
                result = std::abs(m_inputBuffer[idx]);
        }

        m_outputQueue.push(result);
        m_indexQueue.push(idx);
        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        double val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<double> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }
    if (!m_indexQueue.empty())
    {
        int val = m_indexQueue.front();
        m_indexQueue.pop();

        std::vector<int> indexData;
        indexData.push_back(val);
        WriteOutputData(indexPort, indexData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool MaxMin_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_MaxMin = std::make_unique<MaxMin>();

    SetDefaultParameters();
    try { m_N         = std::stoi(getParameter("N").Value);          } catch (...) {}
    try { m_MaxOrMin  = ConvertStringToMaxOrMin(getParameter("MaxOrMin").Value);   } catch (...) {}
    try { m_Compare   = ConvertStringToCompare(getParameter("Compare").Value);     } catch (...) {}
    try { m_OutputType = ConvertStringToOutputType(getParameter("OutputType").Value); } catch (...) {}
    SetParameters();

    if (m_N < 1)
    {
        LOG_ERROR("MaxMin: N must be >= 1.");
        return false;
    }

    AddInputPort("input",  m_MaxMin->input,  static_cast<size_t>(m_N), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_MaxMin->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("index",  m_MaxMin->index,  1, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void MaxMin_Block::SetDefaultParameters()
{
    m_N          = 10;
    m_MaxOrMin   = MaxMin::min;
    m_Compare    = MaxMin::valueIn;
    m_OutputType = MaxMin::valueOut;
}

void MaxMin_Block::SetParameters()
{
    if (!m_MaxMin) return;
    m_MaxMin->N          = m_N;
    m_MaxMin->MaxOrMin   = m_MaxOrMin;
    m_MaxMin->Compare    = m_Compare;
    m_MaxMin->OutputType = m_OutputType;
}

// ============================================================================
// 枚举字符串转换
// ============================================================================

MaxMin::SelectedMaxOrMin MaxMin_Block::ConvertStringToMaxOrMin(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "min" || lower == "0") return MaxMin::min;
    if (lower == "max" || lower == "1") return MaxMin::max;
    return MaxMin::min;
}

MaxMin::SelectedCompare MaxMin_Block::ConvertStringToCompare(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "valuein"  || lower == "0") return MaxMin::valueIn;
    if (lower == "magnitudein" || lower == "1") return MaxMin::magnitudeIn;
    return MaxMin::valueIn;
}

MaxMin::SelectedOutputType MaxMin_Block::ConvertStringToOutputType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "valueout"     || lower == "0") return MaxMin::valueOut;
    if (lower == "magnitudeout" || lower == "1") return MaxMin::magnitudeOut;
    return MaxMin::valueOut;
}
