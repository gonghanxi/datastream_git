#include "RADAR_EWJamming_Block.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <random>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_EWJamming_Block::RADAR_EWJamming_Block(const std::string& name)
    : Block(name)
    , m_rng(std::random_device{}())
{
}

// ============================================================================
// 默认参数
// ============================================================================

void RADAR_EWJamming_Block::SetDefaultParameters()
{
    m_SampleNum   = 1000;
    m_SampleRate  = 10e6;
    m_Mean        = 0.0;
    m_Stdev       = 1.0;
    m_System_Loss = 0.0;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void RADAR_EWJamming_Block::SetParameters()
{
    if (!m_algo) { return; }

    m_algo->SampleNum   = m_SampleNum;
    m_algo->SampleRate  = m_SampleRate;
    m_algo->Mean        = m_Mean;
    m_algo->Stdev       = m_Stdev;
    m_algo->System_Loss = m_System_Loss;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool RADAR_EWJamming_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_EWJamming_Block::Run()
{
    return DataStreamRun();
}

bool RADAR_EWJamming_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);
    m_algo = std::make_unique<RADAR_EWJamming>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_SampleNum   = std::stoi(getParameter("SampleNum").Value);   } catch (...) {}
    try { m_SampleRate  = std::stod(getParameter("SampleRate").Value);  } catch (...) {}
    try { m_Mean        = std::stod(getParameter("Mean").Value);        } catch (...) {}
    try { m_Stdev       = std::stod(getParameter("Stdev").Value);       } catch (...) {}
    try { m_System_Loss = std::stod(getParameter("System_Loss").Value); } catch (...) {}

    SetParameters();

    // ---- 注册端口 ----
    AddOutputPort("jamming", m_algo->jamming, static_cast<size_t>(m_SampleNum),
                  Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑（生成高斯噪声干扰）
// ============================================================================

bool RADAR_EWJamming_Block::DataStreamRun()
{
    SetParameters();

    // 生成高斯噪声
    std::normal_distribution<double> distRe(m_Mean, m_Stdev);
    std::normal_distribution<double> distIm(m_Mean, m_Stdev);

    const double lossFactor = std::pow(10.0, -m_System_Loss / 20.0);

    std::vector<std::complex<double>> outputData;
    outputData.reserve(static_cast<size_t>(m_SampleNum));

    for (int i = 0; i < m_SampleNum; ++i) {
        std::complex<double> noise(distRe(m_rng), distIm(m_rng));
        outputData.push_back(noise * lossFactor);
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}
