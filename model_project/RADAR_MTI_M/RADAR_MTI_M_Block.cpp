#include "RADAR_MTI_M_Block.h"

#include <algorithm>
#include <cctype>
#include <string>

// ============================================================================
// 匿名命名空间 — 纯静态工具函数
// ============================================================================

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

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_MTI_M_Block::RADAR_MTI_M_Block(const std::string& name)
    : Block(name)
    , m_MTI_Type(RADAR_MTI_M::TwoPulseCanceller)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_MTI_M_Block::SetDefaultParameters()
{
    m_MTI_Type = RADAR_MTI_M::TwoPulseCanceller;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_MTI_M_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->MTI_Type = m_MTI_Type;
}

// ============================================================================
// ConvertStringToMTIType
// ============================================================================

RADAR_MTI_M::SelectedMTI_Type
RADAR_MTI_M_Block::ConvertStringToMTIType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "two pulse canceller" || v == "twopulsecanceller" || v == "0") return RADAR_MTI_M::TwoPulseCanceller;
    if (v == "three pulse canceller" || v == "threepulsecanceller" || v == "1") return RADAR_MTI_M::ThreePulseCanceller;
    return RADAR_MTI_M::TwoPulseCanceller;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_MTI_M_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_MTI_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_MTI_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const auto& inMat = inputData[0];

    const int nRows = static_cast<int>(inMat.NumRows());
    const int nCols = static_cast<int>(inMat.NumColumns());

    if (nRows <= 0 || nCols <= 0) return false;
    if (m_MTI_Type == RADAR_MTI_M::TwoPulseCanceller && nCols < 2) return false;
    if (m_MTI_Type == RADAR_MTI_M::ThreePulseCanceller && nCols < 3) return false;

    SystemVueModelBuilder::DComplexMatrix outMat;

    switch (m_MTI_Type)
    {
    case RADAR_MTI_M::TwoPulseCanceller:
    {
        const int outCols = nCols - 1;
        outMat.Resize(nRows, outCols);
        for (int row = 0; row < nRows; ++row) {
            for (int pulse = 1; pulse < nCols; ++pulse) {
                outMat(row, pulse - 1) = inMat(row, pulse) - inMat(row, pulse - 1);
            }
        }
        break;
    }
    case RADAR_MTI_M::ThreePulseCanceller:
    {
        const int outCols = nCols - 2;
        outMat.Resize(nRows, outCols);
        for (int row = 0; row < nRows; ++row) {
            for (int pulse = 2; pulse < nCols; ++pulse) {
                outMat(row, pulse - 2) = inMat(row, pulse)
                    - inMat(row, pulse - 1) * 2.0
                    + inMat(row, pulse - 2);
            }
        }
        break;
    }
    default: return false;
    }

    std::vector<SystemVueModelBuilder::DComplexMatrix> outVec;
    outVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outVec);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_MTI_M_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
        if (inputData.empty()) return true;
        m_inputBuffer.push_back(inputData[0]);
    }

    // ② 判断阈值（rate=1），处理
    if (!m_inputBuffer.empty()) {
        const auto& inMat = m_inputBuffer.front();

        const int nRows = static_cast<int>(inMat.NumRows());
        const int nCols = static_cast<int>(inMat.NumColumns());

        SystemVueModelBuilder::DComplexMatrix outMat;

        bool valid = (nRows > 0 && nCols > 0);
        if (m_MTI_Type == RADAR_MTI_M::TwoPulseCanceller && nCols < 2) valid = false;
        if (m_MTI_Type == RADAR_MTI_M::ThreePulseCanceller && nCols < 3) valid = false;

        if (valid) {
            switch (m_MTI_Type)
            {
            case RADAR_MTI_M::TwoPulseCanceller:
            {
                const int outCols = nCols - 1;
                outMat.Resize(nRows, outCols);
                for (int row = 0; row < nRows; ++row) {
                    for (int pulse = 1; pulse < nCols; ++pulse) {
                        outMat(row, pulse - 1) = inMat(row, pulse) - inMat(row, pulse - 1);
                    }
                }
                break;
            }
            case RADAR_MTI_M::ThreePulseCanceller:
            {
                const int outCols = nCols - 2;
                outMat.Resize(nRows, outCols);
                for (int row = 0; row < nRows; ++row) {
                    for (int pulse = 2; pulse < nCols; ++pulse) {
                        outMat(row, pulse - 2) = inMat(row, pulse)
                            - inMat(row, pulse - 1) * 2.0
                            + inMat(row, pulse - 2);
                    }
                }
                break;
            }
            default: break;
            }
        }

        m_outputQueue.push(outMat);
        m_inputBuffer.erase(m_inputBuffer.begin());
    }

    // ③ 出队写入
    if (!m_outputQueue.empty()) {
        std::vector<SystemVueModelBuilder::DComplexMatrix> outVec;
        outVec.push_back(m_outputQueue.front());
        WriteOutputData(GetOutputPortName(0), outVec);
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_MTI_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_MTI_M>();

    SetDefaultParameters();

    try { m_MTI_Type = ConvertStringToMTIType(getParameter("MTI_Type").Value); } catch (...) {}

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}
