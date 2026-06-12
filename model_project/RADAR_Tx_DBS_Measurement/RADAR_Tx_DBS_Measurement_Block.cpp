#include "RADAR_Tx_DBS_Measurement_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

// ============================================================================
// 字符串处理
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
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Tx_DBS_Measurement_Block::RADAR_Tx_DBS_Measurement_Block(const std::string& name)
    : Block(name)
    , m_PRI(1e-4)
    , m_SamplingRate(10e6)
    , m_NumOfAntx(4)
    , m_NumOfAnty(4)
    , m_Dx(0.5)
    , m_Dy(0.5)
    , m_ParamToSweep(RADAR_Tx_DBS_Measurement::Sweep_Phi)
    , m_Theta_Phi(0.0)
    , m_TypeOfSweep(RADAR_Tx_DBS_Measurement::Linear_Number_of_Points)
    , m_SweepStart(0.0)
    , m_SweepStop(0.0)
    , m_SweepNumOfPoints(360)
    , m_SweepStepSize(0.0)
    , m_nAnt(16)
    , m_inputRate(1000)
    , m_sweepSamples(360)
    , m_sweepStepRad(0.0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Tx_DBS_Measurement_Block::SetDefaultParameters()
{
    m_PRI              = 1e-4;
    m_SamplingRate     = 10e6;
    m_NumOfAntx        = 4;
    m_NumOfAnty        = 4;
    m_Dx               = 0.5;
    m_Dy               = 0.5;
    m_ParamToSweep     = RADAR_Tx_DBS_Measurement::Sweep_Phi;
    m_Theta_Phi        = 0.0;
    m_TypeOfSweep      = RADAR_Tx_DBS_Measurement::Linear_Number_of_Points;
    m_SweepStart       = 0.0;
    m_SweepStop        = 0.0;
    m_SweepNumOfPoints = 360;
    m_SweepStepSize    = 0.0;
}

// ============================================================================
// SetParameters
// ============================================================================

void RADAR_Tx_DBS_Measurement_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->PRI              = m_PRI;
    m_algo->SamplingRate     = m_SamplingRate;
    m_algo->NumOfAntx        = m_NumOfAntx;
    m_algo->NumOfAnty        = m_NumOfAnty;
    m_algo->Dx               = m_Dx;
    m_algo->Dy               = m_Dy;
    m_algo->ParamToSweep     = m_ParamToSweep;
    m_algo->Theta_Phi        = m_Theta_Phi;
    m_algo->TypeOfSweep      = m_TypeOfSweep;
    m_algo->SweepStart       = m_SweepStart;
    m_algo->SweepStop        = m_SweepStop;
    m_algo->SweepNumOfPoints = m_SweepNumOfPoints;
    m_algo->SweepStepSize    = m_SweepStepSize;
}

// ============================================================================
// validateAndPrepare — 对齐原算法 Setup 校验逻辑
// ============================================================================

bool RADAR_Tx_DBS_Measurement_Block::validateAndPrepare()
{
    if (m_PRI <= 0.0)
    {
        LOG_ERROR("PRI must be greater than 0.");
        return false;
    }
    if (m_SamplingRate <= 0.0)
    {
        LOG_ERROR("SamplingRate must be greater than 0.");
        return false;
    }

    if (m_NumOfAntx < 1) m_NumOfAntx = 1;
    if (m_NumOfAnty < 1) m_NumOfAnty = 1;

    m_nAnt = m_NumOfAntx * m_NumOfAnty;
    if (m_nAnt < 1) m_nAnt = 1;

    // inputRate = round(PRI * SamplingRate)
    {
        const double v = m_PRI * m_SamplingRate;
        if (!(v > 0.0) || !std::isfinite(v)) { LOG_ERROR("Invalid PRI*SamplingRate."); return false; }
        m_inputRate = static_cast<int>(std::floor(v + 0.5));
        if (m_inputRate < 1) { LOG_ERROR("Input rate must be >= 1."); return false; }
    }

    // sweepSamples
    if (m_TypeOfSweep == RADAR_Tx_DBS_Measurement::Linear_Number_of_Points)
    {
        if (m_SweepNumOfPoints < 1) { LOG_ERROR("SweepNumOfPoints must be >= 1."); return false; }
        m_sweepSamples = m_SweepNumOfPoints;
    }
    else
    {
        const double span = m_SweepStop - m_SweepStart;
        const double stepAbs = std::abs(m_SweepStepSize);
        if (!(stepAbs > 0.0) || !std::isfinite(stepAbs)) { LOG_ERROR("SweepStepSize must be > 0."); return false; }
        const double n = std::ceil(std::abs(span) / stepAbs);
        if (!(n > 0.0) || !std::isfinite(n)) { LOG_ERROR("Invalid sweep range."); return false; }
        m_sweepSamples = static_cast<int>(n);
    }

    // sweepStepRad
    if (m_TypeOfSweep == RADAR_Tx_DBS_Measurement::Linear_Number_of_Points)
    {
        const double span = m_SweepStop - m_SweepStart;
        m_sweepStepRad = (m_sweepSamples > 0) ? (span / static_cast<double>(m_sweepSamples)) : 0.0;
    }
    else
    {
        const double stepAbs = std::abs(m_SweepStepSize);
        if (!(stepAbs > 0.0) || !std::isfinite(stepAbs)) { LOG_ERROR("SweepStepSize must be > 0."); return false; }
        m_sweepStepRad = (m_SweepStop >= m_SweepStart) ? stepAbs : -stepAbs;
    }

    return true;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Tx_DBS_Measurement_Block::Setup()
{
    Block::Setup();

    m_inputBusBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Advance — TimedDFModel 需要驱动时间轴
// ============================================================================

void RADAR_Tx_DBS_Measurement_Block::Advance()
{
    if (m_algo) m_algo->Advance();
}

// ============================================================================
// Run
// ============================================================================

bool RADAR_Tx_DBS_Measurement_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Tx_DBS_Measurement_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Tx_DBS_Measurement>();

    SetDefaultParameters();

    try { m_PRI              = std::stod(getParameter("PRI").Value);              } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
    try { m_SamplingRate     = std::stod(getParameter("SamplingRate").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'SamplingRate', using default value."); }
    try { m_NumOfAntx        = std::stoi(getParameter("NumOfAntx").Value);        } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAntx', using default value."); }
    try { m_NumOfAnty        = std::stoi(getParameter("NumOfAnty").Value);        } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAnty', using default value."); }
    try { m_Dx               = std::stod(getParameter("Dx").Value);               } catch (...) { LOG_WARN("Failed to parse parameter 'Dx', using default value."); }
    try { m_Dy               = std::stod(getParameter("Dy").Value);               } catch (...) { LOG_WARN("Failed to parse parameter 'Dy', using default value."); }
    try { m_ParamToSweep     = ConvertStringToParamToSweep(getParameter("ParamToSweep").Value);   } catch (...) { LOG_WARN("Failed to parse parameter 'ParamToSweep', using default value."); }
    try { m_Theta_Phi        = std::stod(getParameter("Theta_Phi").Value);        } catch (...) { LOG_WARN("Failed to parse parameter 'Theta_Phi', using default value."); }
    try { m_TypeOfSweep      = ConvertStringToTypeOfSweep(getParameter("TypeOfSweep").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'TypeOfSweep', using default value."); }
    try { m_SweepStart       = std::stod(getParameter("SweepStart").Value);       } catch (...) { LOG_WARN("Failed to parse parameter 'SweepStart', using default value."); }
    try { m_SweepStop        = std::stod(getParameter("SweepStop").Value);        } catch (...) { LOG_WARN("Failed to parse parameter 'SweepStop', using default value."); }
    try { m_SweepNumOfPoints = std::stoi(getParameter("SweepNumOfPoints").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SweepNumOfPoints', using default value."); }
    try { m_SweepStepSize    = std::stod(getParameter("SweepStepSize").Value);    } catch (...) { LOG_WARN("Failed to parse parameter 'SweepStepSize', using default value."); }

    SetParameters();

    if (!validateAndPrepare()) {
        return false;
    }

    // Input: envelope bus (nAnt 路，每路 inputRate 个 token)
    AddInputPort("input",      m_algo->input,      m_inputRate,    Block::DataType::ENVELOPE_BUS);
    // Output: complex
    AddOutputPort("AntPattern", m_algo->AntPattern, m_sweepSamples, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// 辅助：获取 sweep 角度
// ============================================================================

static void getThetaPhiForSweep(
    int k, int sweepSamples, double sweepStepRad,
    RADAR_Tx_DBS_Measurement::ParamToSweepEnum paramToSweep,
    RADAR_Tx_DBS_Measurement::TypeOfSweepEnum  typeOfSweep,
    double thetaPhi, double sweepStart, double sweepStop,
    double& thetaRad, double& phiRad, double& sweepAngleRad)
{
    sweepAngleRad = sweepStart + static_cast<double>(k) * sweepStepRad;

    if (typeOfSweep == RADAR_Tx_DBS_Measurement::Linear_Number_of_Points &&
        sweepSamples > 1 && k == sweepSamples - 1)
    {
        sweepAngleRad = sweepStop;
    }

    if (paramToSweep == RADAR_Tx_DBS_Measurement::Sweep_Phi)
    {
        thetaRad = thetaPhi;
        phiRad = sweepAngleRad;
    }
    else
    {
        thetaRad = sweepAngleRad;
        phiRad = thetaPhi;
    }
}

// ============================================================================
// 辅助：计算相位
// ============================================================================

static double computePhaseRad(int kx, int ky, double Dx, double Dy,
                               double thetaRad, double phiRad)
{
    const double sx = std::sin(thetaRad) * std::cos(phiRad);
    const double sy = std::sin(thetaRad) * std::sin(phiRad);
    return 2.0 * M_PI * (static_cast<double>(kx) * Dx * sx + static_cast<double>(ky) * Dy * sy);
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_Tx_DBS_Measurement_Block::DataStreamRun()
{
    std::string inputName = GetInputPortName(0);
    auto inputData = ReadInputData<EnvelopeSignal>(inputName);

    if (inputData.empty()) return true;

    const int nAntRead = m_nAnt;
    const int R = m_inputRate;

    if (static_cast<int>(inputData.size()) != nAntRead * R)
    {
        LOG_ERROR("Unexpected input data size.");
        return false;
    }

    // 运行 sweep 计算
    // inputData 是 flat vector，lane-major 布局: lane0_sample0, lane0_sample1, ..., lane0_sample(R-1), lane1_sample0, ...
    std::vector<Cx> outVec(static_cast<size_t>(m_sweepSamples));

    for (int k = 0; k < m_sweepSamples; ++k)
    {
        double thetaRad = 0.0, phiRad = 0.0, sweepAngleRad = 0.0;
        getThetaPhiForSweep(k, m_sweepSamples, m_sweepStepRad,
                            m_ParamToSweep, m_TypeOfSweep,
                            m_Theta_Phi, m_SweepStart, m_SweepStop,
                            thetaRad, phiRad, sweepAngleRad);

        Cx acc(0.0, 0.0);

        for (int i = 0; i < R; ++i)
        {
            for (int ky = 0; ky < m_NumOfAnty; ++ky)
            {
                for (int kx = 0; kx < m_NumOfAntx; ++kx)
                {
                    const int antIndex = kx + m_NumOfAntx * ky;
                    if (antIndex < 0 || antIndex >= nAntRead) continue;

                    const double phase = computePhaseRad(kx, ky, m_Dx, m_Dy, thetaRad, phiRad);
                    const Cx rot(std::cos(phase), std::sin(phase));

                    // Flat vector 索引: antIndex * R + i
                    const size_t idx = static_cast<size_t>(antIndex * R + i);
                    acc += inputData[idx].complex() * rot;
                }
            }
        }

        Cx avg = acc;
        if (R > 0)
            avg /= static_cast<double>(R);

        const double power = std::norm(avg);
        const Cx anglePoint(std::cos(sweepAngleRad), std::sin(sweepAngleRad));
        outVec[static_cast<size_t>(k)] = power * anglePoint;
    }

    WriteOutputData(GetOutputPortName(0), outVec);
    return true;
}

// ============================================================================
// TimeDrivenRun
// ============================================================================

bool RADAR_Tx_DBS_Measurement_Block::TimeDrivenRun()
{
    // ① 累积输入：使用 ReadInputData 读取 flat vector
    {
        std::string inputName = GetInputPortName(0);
        auto data = ReadInputData<EnvelopeSignal>(inputName);
        for (auto& sig : data)
            m_inputBusBuffer.push_back(sig);
    }

    const int nAntRead = m_nAnt;
    const int R = m_inputRate;
    const int requiredTokens = nAntRead * R;

    // ② 当累积足够时，处理一个完整 PRI 的数据并生成所有 sweep 输出
    if (static_cast<int>(m_inputBusBuffer.size()) >= requiredTokens)
    {
        for (int k = 0; k < m_sweepSamples; ++k)
        {
            double thetaRad = 0.0, phiRad = 0.0, sweepAngleRad = 0.0;
            getThetaPhiForSweep(k, m_sweepSamples, m_sweepStepRad,
                                m_ParamToSweep, m_TypeOfSweep,
                                m_Theta_Phi, m_SweepStart, m_SweepStop,
                                thetaRad, phiRad, sweepAngleRad);

            Cx acc(0.0, 0.0);

            for (int i = 0; i < R; ++i)
            {
                for (int ky = 0; ky < m_NumOfAnty; ++ky)
                {
                    for (int kx = 0; kx < m_NumOfAntx; ++kx)
                    {
                        const int antIndex = kx + m_NumOfAntx * ky;
                        if (antIndex < 0 || antIndex >= nAntRead) continue;

                        const double phase = computePhaseRad(kx, ky, m_Dx, m_Dy, thetaRad, phiRad);
                        const Cx rot(std::cos(phase), std::sin(phase));

                        // Flat vector 索引: antIndex * R + i
                        const size_t idx = static_cast<size_t>(antIndex * R + i);
                        acc += m_inputBusBuffer[idx].complex() * rot;
                    }
                }
            }

            Cx avg = acc;
            if (R > 0) avg /= static_cast<double>(R);

            const double power = std::norm(avg);
            const Cx anglePoint(std::cos(sweepAngleRad), std::sin(sweepAngleRad));
            m_outputQueue.push(power * anglePoint);
        }

        // 清空已处理的输入
        m_inputBusBuffer.clear();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty())
    {
        Cx v = m_outputQueue.front(); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<Cx>{v});
    }

    return true;
}

// ============================================================================
// ConvertStringToParamToSweep
// ============================================================================

RADAR_Tx_DBS_Measurement::ParamToSweepEnum
RADAR_Tx_DBS_Measurement_Block::ConvertStringToParamToSweep(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "0" || v == "phi" || v == "sweep_phi")   return RADAR_Tx_DBS_Measurement::Sweep_Phi;
    if (v == "1" || v == "theta" || v == "sweep_theta") return RADAR_Tx_DBS_Measurement::Sweep_Theta;

    try { return static_cast<RADAR_Tx_DBS_Measurement::ParamToSweepEnum>(std::stoi(value)); } catch (...) {}
    return RADAR_Tx_DBS_Measurement::Sweep_Phi;
}

// ============================================================================
// ConvertStringToTypeOfSweep
// ============================================================================

RADAR_Tx_DBS_Measurement::TypeOfSweepEnum
RADAR_Tx_DBS_Measurement_Block::ConvertStringToTypeOfSweep(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "0" || v == "linear_number_of_points" || v == "linear:number of points")
        return RADAR_Tx_DBS_Measurement::Linear_Number_of_Points;
    if (v == "1" || v == "linear_step_size" || v == "linear:step size")
        return RADAR_Tx_DBS_Measurement::Linear_Step_Size;

    try { return static_cast<RADAR_Tx_DBS_Measurement::TypeOfSweepEnum>(std::stoi(value)); } catch (...) {}
    return RADAR_Tx_DBS_Measurement::Linear_Number_of_Points;
}
