#include "RADAR_EWDeceptionJamming_Block.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 构造函数
// ============================================================================

RADAR_EWDeceptionJamming_Block::RADAR_EWDeceptionJamming_Block(const std::string& name)
    : Block(name)
    , m_MaxSampleNum(0)
    , m_SampleIndex(0)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void RADAR_EWDeceptionJamming_Block::SetDefaultParameters()
{
    m_SampleNum       = 1000;
    m_SampleRate      = 10e6;
    m_FalseTargetNum  = 1;
    m_MaxRange        = 100e3;
    m_System_Loss     = 0.0;

    m_FalseTargetRangeDelay.Resize(1, 1);
    m_FalseTargetRangeDelay(0, 0) = 100.0;

    m_FalseTargetDopplerOffset.Resize(1, 1);
    m_FalseTargetDopplerOffset(0, 0) = 0.0;

    m_FalseTargetGain.Resize(1, 1);
    m_FalseTargetGain(0, 0) = 1.0;

    m_MaxSampleNum = 0;
    m_SampleIndex  = 0;
    m_FalseTargetDelayBuffer.Resize(1, 1);
    m_FalseTargetDelayBuffer.Zero();
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void RADAR_EWDeceptionJamming_Block::SetParameters()
{
    if (!m_algo) { return; }

    m_algo->SampleNum       = m_SampleNum;
    m_algo->SampleRate      = m_SampleRate;
    m_algo->FalseTargetNum  = m_FalseTargetNum;
    m_algo->MaxRange        = m_MaxRange;
    m_algo->System_Loss     = m_System_Loss;
    m_algo->FalseTargetRangeDelay    = m_FalseTargetRangeDelay;
    m_algo->FalseTargetDopplerOffset = m_FalseTargetDopplerOffset;
    m_algo->FalseTargetGain          = m_FalseTargetGain;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool RADAR_EWDeceptionJamming_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_EWDeceptionJamming_Block::Run()
{
    return DataStreamRun();
}

bool RADAR_EWDeceptionJamming_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<RADAR_EWDeceptionJamming>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_SampleNum      = std::stoi(getParameter("SampleNum").Value);              } catch (...) {}
    try { m_SampleRate     = std::stod(getParameter("SampleRate").Value);             } catch (...) {}
    try { m_FalseTargetNum = std::stoi(getParameter("FalseTargetNum").Value);         } catch (...) {}
    try { m_MaxRange       = std::stod(getParameter("MaxRange").Value);               } catch (...) {}
    try { m_System_Loss    = std::stod(getParameter("System_Loss").Value);            } catch (...) {}
    try { m_FalseTargetRangeDelay    = ParseStringToMatrix<double>(
              getParameter("FalseTargetRangeDelay").Value);                           } catch (...) {}
    try { m_FalseTargetDopplerOffset = ParseStringToMatrix<double>(
              getParameter("FalseTargetDopplerOffset").Value);                       } catch (...) {}
    try { m_FalseTargetGain          = ParseStringToMatrix<double>(
              getParameter("FalseTargetGain").Value);                                } catch (...) {}

    SetParameters();

    // ---- 初始化内部状态 ----
    const double c = 3e8;
    m_MaxSampleNum = static_cast<int>(2.0 * m_MaxRange / c * m_SampleRate);
    m_FalseTargetDelayBuffer.Resize(1, m_MaxSampleNum);
    m_FalseTargetDelayBuffer.Zero();
    m_SampleIndex = 0;

    // ---- 注册端口 ----
    AddInputPort("signal",   m_algo->signal,   static_cast<size_t>(m_SampleNum),
                 Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("jamming", m_algo->jamming,   static_cast<size_t>(m_SampleNum),
                  Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool RADAR_EWDeceptionJamming_Block::DataStreamRun()
{
    SetParameters();

    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if (inputData.empty()) { return false; }

    const double c  = 3e8;
    const double PI = std::acos(-1.0);
    const std::complex<double> imag_I(0.0, 1.0);

    const int sampleNum = static_cast<int>(inputData.size());

    // 将假目标信号写入延迟缓冲区
    for (int n = 0; n < m_FalseTargetNum; ++n) {
        const int NDelay = static_cast<int>(
            2.0 * m_FalseTargetRangeDelay(n) / c * m_SampleRate);

        for (int i = 0; i < sampleNum; ++i) {
            const int idx = m_SampleIndex + i + NDelay;
            if (idx < m_MaxSampleNum) {
                m_FalseTargetDelayBuffer(idx) +=
                    inputData[static_cast<size_t>(i)]
                    * std::exp(-imag_I * 2.0 * PI * m_FalseTargetDopplerOffset(n))
                    * m_FalseTargetGain(n);
            }
        }
    }

    // 从延迟缓冲区读出欺骗信号
    std::vector<std::complex<double>> outputData;
    outputData.reserve(static_cast<size_t>(sampleNum));

    for (int i = 0; i < sampleNum; ++i) {
        outputData.push_back(m_FalseTargetDelayBuffer(m_SampleIndex + i));
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    // 推进采样索引
    m_SampleIndex += sampleNum;

    // 如果超出最大范围则重置
    if (m_SampleIndex > m_MaxSampleNum) {
        m_FalseTargetDelayBuffer.Zero();
        m_SampleIndex = 0;
    }

    return true;
}
