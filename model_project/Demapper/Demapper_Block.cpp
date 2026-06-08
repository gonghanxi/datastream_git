#include "Demapper_Block.h"

#include <algorithm>
#include <cctype>
#include <complex>
#include <limits>
#include <vector>

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

Demapper_Block::Demapper_Block(const std::string& name)
    : Block(name)
    , m_ModType(SystemVueModelBuilder::Demapper::QPSK)
    , m_BitOrder(SystemVueModelBuilder::Demapper::LSB_first)
    , m_Ratio_R2_R1(2.0)
    , m_Ratio_R3_R1(3.0)
    , m_Ratio_R4_R1(4.0)
    , m_DefaultState(SystemVueModelBuilder::Demapper::TRUE_)
    , m_symbolLength(2)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void Demapper_Block::SetDefaultParameters()
{
    m_ModType      = SystemVueModelBuilder::Demapper::QPSK;
    m_BitOrder     = SystemVueModelBuilder::Demapper::LSB_first;
    m_Ratio_R2_R1  = 2.0;
    m_Ratio_R3_R1  = 3.0;
    m_Ratio_R4_R1  = 4.0;
    m_DefaultState = SystemVueModelBuilder::Demapper::TRUE_;
    m_symbolLength = 2;
    m_M            = 4;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void Demapper_Block::SetParameters()
{
    if (!m_demapper) { return; }

    m_demapper->ModType        = m_ModType;
    m_demapper->BitOrder       = m_BitOrder;
    m_demapper->Ratio_R2_R1    = m_Ratio_R2_R1;
    m_demapper->Ratio_R3_R1    = m_Ratio_R3_R1;
    m_demapper->Ratio_R4_R1    = m_Ratio_R4_R1;
    m_demapper->DefaultState   = m_DefaultState;
    m_demapper->MappingTable   = m_MappingTable;
    m_demapper->RingStates     = m_RingStates;
    m_demapper->RingMagnitudes = m_RingMagnitudes;
    m_demapper->RinginitialPhases = m_RinginitialPhases;
    m_demapper->States         = m_States;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool Demapper_Block::Setup()
{
    Block::Setup();

    m_inputQueue = std::queue<std::complex<double>>();
    m_bitsQueue  = std::queue<bool>();
    m_nodeQueue  = std::queue<std::complex<double>>();

    bool bStatus = true;

    if (m_symbolLength <= 0)
    {
        LOG_ERROR("symbolLength must be > 0");
        bStatus = false;
    }
    if (m_M <= 0)
    {
        LOG_ERROR("constellationSize (M) must be > 0");
        bStatus = false;
    }

    return bStatus;
}

bool Demapper_Block::Run()
{
    if (IsVariableStepMode() || m_symbolLength > 1) { return TimeDrivenRun(); }
    return DataStreamRun();
}

bool Demapper_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_demapper = std::make_unique<SystemVueModelBuilder::Demapper>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_ModType      = ConvertStringToModTypeEnum(getParameter("ModType").Value);    } catch (...) {}
    try { m_BitOrder     = ConvertStringToBitOrderEnum(getParameter("BitOrder").Value);  } catch (...) {}
    try { m_Ratio_R2_R1  = std::stod(getParameter("Ratio_R2_R1").Value);                } catch (...) {}
    try { m_Ratio_R3_R1  = std::stod(getParameter("Ratio_R3_R1").Value);                } catch (...) {}
    try { m_Ratio_R4_R1  = std::stod(getParameter("Ratio_R4_R1").Value);                } catch (...) {}
    try { m_DefaultState = ConvertStringToDefaultStateEnum(getParameter("DefaultState").Value); } catch (...) {}

    try { m_MappingTable   = ParseStringToMatrix<std::complex<double>>(getParameter("MappingTable").Value);    } catch (...) {}
    try { m_RingStates     = ParseStringToMatrix<int>(getParameter("RingStates").Value);                       } catch (...) {}
    try { m_RingMagnitudes = ParseStringToMatrix<double>(getParameter("RingMagnitudes").Value);                } catch (...) {}
    try { m_RinginitialPhases = ParseStringToMatrix<double>(getParameter("RinginitialPhases").Value);          } catch (...) {}
    try { m_States         = ParseStringToMatrix<int>(getParameter("States").Value);                           } catch (...) {}

    SetParameters();

    // ---- 调用 Demapper::Setup 构建星座图 ----
    if (!m_demapper->Setup()) {
        return false;
    }

    // ---- 从 Demapper 获取构建好的内部状态 ----
    m_symbolLength       = m_demapper->GetSymbolLength();
    m_M                  = m_demapper->GetConstellationSize();
    m_constellationTable = m_demapper->GetConstellationTable();
    m_indexToState       = m_demapper->GetIndexToState();

    // 缓存默认值
    if (m_symbolLength <= 0) { m_symbolLength = 2; }

    // ---- 注册端口 ----
    // m_input : CircularBuffer<complex<double>>  → CIRCULAR_BUFFER_DCOMPLEX
    // m_bits  : CircularBuffer<bool>             → CIRCULAR_BUFFER_BOOL
    // m_node  : CircularBuffer<complex<double>>  → CIRCULAR_BUFFER_DCOMPLEX
    AddInputPort("m_input", m_demapper->m_input, 1,                        Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("m_bits", m_demapper->m_bits,  m_symbolLength,           Block::DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("m_node", m_demapper->m_node,  1,                        Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool Demapper_Block::DataStreamRun()
{
    SetParameters();

    // ---- 读取输入符号 ----
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if (inputData.empty()) { return false; }

    const int inputSize    = static_cast<int>(inputData.size());
    const int symbolLength = m_symbolLength;
    const int M            = m_M;

    std::vector<bool>                    outputBitsData;
    std::vector<std::complex<double>>    outputNodeData;
    outputBitsData.reserve(static_cast<size_t>(inputSize * symbolLength));
    outputNodeData.reserve(static_cast<size_t>(inputSize));

    // ---- 逐符号解调：在 Block 中实现完整逻辑 ----
    for (int i = 0; i < inputSize; ++i) {
        const std::complex<double> x = inputData[static_cast<size_t>(i)];

        // 查找最近星座点 (复制自 Demapper::FindNearestIndex)
        int bestIdx = -1;
        double bestDist = std::numeric_limits<double>::infinity();

        for (int j = 0; j < M; ++j) {
            const double d = std::norm(x - m_constellationTable[static_cast<size_t>(j)]);
            if (d < bestDist) {
                bestDist = d;
                bestIdx = j;
            }
        }

        if (bestIdx < 0 || bestIdx >= M) {
            LOG_ERROR("Demapper internal error: invalid nearest constellation index.");
            return false;
        }

        // 输出解调后的符号
        outputNodeData.push_back(m_constellationTable[static_cast<size_t>(bestIdx)]);

        // 获取状态 (复制自 Demapper::Run)
        int state = bestIdx;
        if (m_ModType == SystemVueModelBuilder::Demapper::CustomAPSK &&
            m_DefaultState == SystemVueModelBuilder::Demapper::FALSE_) {
            if (static_cast<int>(m_indexToState.size()) == M) {
                state = m_indexToState[static_cast<size_t>(bestIdx)];
            }
        }

        // 根据 BitOrder 输出比特 (复制自 Demapper::WriteBitsFromState)
        if (m_BitOrder == SystemVueModelBuilder::Demapper::LSB_first) {
            for (int k = 0; k < symbolLength; ++k) {
                outputBitsData.push_back(((state >> k) & 0x1) != 0);
            }
        } else {
            for (int k = 0; k < symbolLength; ++k) {
                outputBitsData.push_back(((state >> (symbolLength - 1 - k)) & 0x1) != 0);
            }
        }
    }

    // ---- 写入输出数据 ----
    if (!outputBitsData.empty()) {
        WriteOutputData(GetOutputPortName(0), outputBitsData);
    }
    if (!outputNodeData.empty()) {
        WriteOutputData(GetOutputPortName(1), outputNodeData);
    }

    return true;
}

// ============================================================================
// TimeDrivenRun：变步长逐点处理 — 输入符号解调为 bits + node，双输出独立出队
// ============================================================================

bool Demapper_Block::TimeDrivenRun()
{
    // ① 累积输入符号
    {
        auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
        for (auto& v : inputData) m_inputQueue.push(v);
    }

    // ② 有输入时 → 解调一个符号
    if (!m_inputQueue.empty())
    {
        const std::complex<double> x = m_inputQueue.front(); m_inputQueue.pop();
        const int symbolLength = m_symbolLength;
        const int M            = m_M;

        // 查找最近星座点
        int bestIdx = -1;
        double bestDist = std::numeric_limits<double>::infinity();
        for (int j = 0; j < M; ++j) {
            const double d = std::norm(x - m_constellationTable[static_cast<size_t>(j)]);
            if (d < bestDist) {
                bestDist = d;
                bestIdx = j;
            }
        }

        if (bestIdx < 0 || bestIdx >= M) {
            LOG_ERROR("Demapper internal error: invalid nearest constellation index.");
            return false;
        }

        // 输出 node 符号
        m_nodeQueue.push(m_constellationTable[static_cast<size_t>(bestIdx)]);

        // 获取状态
        int state = bestIdx;
        if (m_ModType == SystemVueModelBuilder::Demapper::CustomAPSK &&
            m_DefaultState == SystemVueModelBuilder::Demapper::FALSE_) {
            if (static_cast<int>(m_indexToState.size()) == M) {
                state = m_indexToState[static_cast<size_t>(bestIdx)];
            }
        }

        // 根据 BitOrder 输出 bits
        if (m_BitOrder == SystemVueModelBuilder::Demapper::LSB_first) {
            for (int k = 0; k < symbolLength; ++k) {
                m_bitsQueue.push(((state >> k) & 0x1) != 0);
            }
        } else {
            for (int k = 0; k < symbolLength; ++k) {
                m_bitsQueue.push(((state >> (symbolLength - 1 - k)) & 0x1) != 0);
            }
        }
    }

    // ③ 出队写入，两个输出分开判断
    bool wroteOutput = false;
    if (!m_bitsQueue.empty()) {
        bool b = m_bitsQueue.front(); m_bitsQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<bool>{b});
        wroteOutput = true;
    }
    if (!m_nodeQueue.empty()) {
        std::complex<double> n = m_nodeQueue.front(); m_nodeQueue.pop();
        WriteOutputData(GetOutputPortName(1), std::vector<std::complex<double>>{n});
        wroteOutput = true;
    }
    if (wroteOutput) {
        m_inputQueue = std::queue<std::complex<double>>();
    }

    return true;
}

// ============================================================================
// 枚举转换
// ============================================================================

Demapper_Block::ModTypeEnum
Demapper_Block::ConvertStringToModTypeEnum(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "bpsk"         || s == "0")  return SystemVueModelBuilder::Demapper::BPSK;
    if (s == "qpsk"         || s == "1")  return SystemVueModelBuilder::Demapper::QPSK;
    if (s == "psk8"         || s == "2")  return SystemVueModelBuilder::Demapper::PSK8;
    if (s == "psk16"        || s == "3")  return SystemVueModelBuilder::Demapper::PSK16;
    if (s == "qam16"        || s == "4")  return SystemVueModelBuilder::Demapper::QAM16;
    if (s == "qam32"        || s == "5")  return SystemVueModelBuilder::Demapper::QAM32;
    if (s == "qam64"        || s == "6")  return SystemVueModelBuilder::Demapper::QAM64;
    if (s == "qam128"       || s == "7")  return SystemVueModelBuilder::Demapper::QAM128;
    if (s == "qam256"       || s == "8")  return SystemVueModelBuilder::Demapper::QAM256;
    if (s == "user_defined" || s == "9")  return SystemVueModelBuilder::Demapper::User_Defined;
    if (s == "qam512"       || s == "10") return SystemVueModelBuilder::Demapper::QAM512;
    if (s == "qam1024"      || s == "11") return SystemVueModelBuilder::Demapper::QAM1024;
    if (s == "qam2048"      || s == "12") return SystemVueModelBuilder::Demapper::QAM2048;
    if (s == "qam4096"      || s == "13") return SystemVueModelBuilder::Demapper::QAM4096;
    if (s == "apsk16"       || s == "14") return SystemVueModelBuilder::Demapper::APSK16;
    if (s == "apsk32"       || s == "15") return SystemVueModelBuilder::Demapper::APSK32;
    if (s == "star16qam"    || s == "16") return SystemVueModelBuilder::Demapper::Star16QAM;
    if (s == "star32qam"    || s == "17") return SystemVueModelBuilder::Demapper::Star32QAM;
    if (s == "customapsk"   || s == "18") return SystemVueModelBuilder::Demapper::CustomAPSK;
    return SystemVueModelBuilder::Demapper::QPSK;
}

Demapper_Block::BitOrderEnum
Demapper_Block::ConvertStringToBitOrderEnum(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "lsbfirst" || s == "0") return SystemVueModelBuilder::Demapper::LSB_first;
    if (s == "msbfirst" || s == "1") return SystemVueModelBuilder::Demapper::MSB_first;
    return SystemVueModelBuilder::Demapper::LSB_first;
}

Demapper_Block::DefaultStateEnum
Demapper_Block::ConvertStringToDefaultStateEnum(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "false" || s == "0") return SystemVueModelBuilder::Demapper::FALSE_;
    if (s == "true"  || s == "1") return SystemVueModelBuilder::Demapper::TRUE_;
    return SystemVueModelBuilder::Demapper::TRUE_;
}
