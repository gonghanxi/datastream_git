#include "FFT_Shift_Block.h"

#include <algorithm>
#include <cctype>

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
}

// ============================================================================
// 构造函数
// ============================================================================

FFT_Shift_Block::FFT_Shift_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool FFT_Shift_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool FFT_Shift_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool FFT_Shift_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);

    if (inputData.empty()) return true;

    std::vector<std::complex<double>> outputData;
    outputData.reserve(static_cast<size_t>(m_FFTSize));

    // Direction == 0: FFTShift, Direction == 1: IFFTShift
    if (m_Direction == 0)
    {
        for (int i = 0; i < m_FFTSize; ++i)
        {
            int n = i - m_FFTSize / 2;
            outputData.push_back(inputData[n >= 0 ? n : n + m_FFTSize]);
        }
    }
    else
    {
        for (int i = 0; i < m_FFTSize; ++i)
        {
            int n = i + m_FFTSize / 2;
            outputData.push_back(inputData[n < m_FFTSize ? n : n - m_FFTSize]);
        }
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool FFT_Shift_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_FFTSize)
    {
        // 生成重新排列后的所有输出并入队
        if (m_Direction == FFT_Shift::FFTShift)
        {
            for (int i = 0; i < m_FFTSize; ++i)
            {
                int n = i - m_FFTSize / 2;
                m_outputQueue.push(m_inputBuffer[n >= 0 ? n : n + m_FFTSize]);
            }
        }
        else
        {
            for (int i = 0; i < m_FFTSize; ++i)
            {
                int n = i + m_FFTSize / 2;
                m_outputQueue.push(m_inputBuffer[n < m_FFTSize ? n : n - m_FFTSize]);
            }
        }

        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        std::complex<double> val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<std::complex<double>> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool FFT_Shift_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_FFT_Shift = std::make_unique<FFT_Shift>();

    SetDefaultParameters();
    try { m_FFTSize   = std::stoi(getParameter("FFTSize").Value);   } catch(...) {}
    try { m_Direction = ConvertStringToDirection(getParameter("Direction").Value); } catch(...) {}
    SetParameters();

    if (m_FFTSize < 1)
    {
        LOG_ERROR("FFTSize should be greater than 1");
        return false;
    }

    AddInputPort("input",  m_FFT_Shift->input,  static_cast<size_t>(m_FFTSize), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_FFT_Shift->output, static_cast<size_t>(m_FFTSize), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void FFT_Shift_Block::SetDefaultParameters()
{
    m_FFTSize   = 256;
    m_Direction = FFT_Shift::FFTShift;
}

void FFT_Shift_Block::SetParameters()
{
    if (!m_FFT_Shift) return;
    m_FFT_Shift->FFTSize   = m_FFTSize;
    m_FFT_Shift->Direction = m_Direction;
}

FFT_Shift::SelectedDirection FFT_Shift_Block::ConvertStringToDirection(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "fftshift" || lower == "0")
    {
        return FFT_Shift::FFTShift;
    }
    if (lower == "ifftshift" || lower == "1")
    {
        return FFT_Shift::IFFTShift;
    }
    return FFT_Shift::FFTShift;
}
