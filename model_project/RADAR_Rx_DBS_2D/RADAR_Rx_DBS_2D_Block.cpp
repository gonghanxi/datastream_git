#include "RADAR_Rx_DBS_2D_Block.h"

#include <algorithm>
#include <cmath>
#include <random>


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

RADAR_Rx_DBS_2D_Block::RADAR_Rx_DBS_2D_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Rx_DBS_2D_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_Rx_DBS_2D_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_Rx_DBS_2D_Block::DataStreamRun()
{
    const int nChExpected = m_nChExpected;

    // ---- 读取 bus 输入 ----
    std::string inputPort = GetInputPortName(0);
    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) return true;

    const int nChannels = static_cast<int>(inputData.size());
    const int nUse = std::min(nChannels, nChExpected);

    // ---- 角度 ----
    double theta = m_Theta;
    double phi   = m_Phi;
    bool thetaFromPort = false;
    bool phiFromPort   = false;

    // InTheta（可选端口，值已是弧度）
    std::string thetaPort = GetInputPortName(1);
    if (GetInputPort(thetaPort)->IsConnected())
    {
        auto thetaData = ReadInputData<double>(thetaPort);
        if (!thetaData.empty()) { theta = thetaData[0]; thetaFromPort = true; }
    }
    // InPhi（可选端口，值已是弧度）
    std::string phiPort = GetInputPortName(2);
    if (GetInputPort(phiPort)->IsConnected())
    {
        auto phiData = ReadInputData<double>(phiPort);
        if (!phiData.empty()) { phi = phiData[0]; phiFromPort = true; }
    }

    // 仅参数默认值（deg）需转 rad，端口值已是 rad 不转换
    if (!thetaFromPort && std::fabs(theta) > kTwoPi) theta = deg2rad(theta);
    if (!phiFromPort   && std::fabs(phi)   > kTwoPi) phi   = deg2rad(phi);

    // ---- 波束合成 ----
    const double sTh = std::sin(theta);
    const double ux  = sTh * std::cos(phi);
    const double uy  = sTh * std::sin(phi);

    std::complex<double> ysum(0.0, 0.0);

    for (int idx = 0; idx < nUse; ++idx)
    {
        const int n = idx / m_nx;
        const int m = idx % m_nx;

        const double psi = kTwoPi * (m_xPos[m] * ux + m_yPos[n] * uy);
        const std::complex<double> phase(std::cos(psi), -std::sin(psi)); // exp(-j*psi)

        ysum += inputData[idx] * (m_taper2d[idx] * phase);
    }

    std::string outputPort = GetOutputPortName(0);
    std::vector<std::complex<double>> outputData;
    outputData.push_back(ysum);
    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool RADAR_Rx_DBS_2D_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_nChExpected)
    {
        // ---- 角度 ----
        double theta = m_Theta;
        double phi   = m_Phi;
        bool thetaFromPort = false;
        bool phiFromPort   = false;

        std::string thetaPort = GetInputPortName(1);
        if (GetInputPort(thetaPort)->IsConnected())
        {
            auto thetaData = ReadInputData<double>(thetaPort);
            if (!thetaData.empty()) { theta = thetaData[0]; thetaFromPort = true; }
        }
        std::string phiPort = GetInputPortName(2);
        if (GetInputPort(phiPort)->IsConnected())
        {
            auto phiData = ReadInputData<double>(phiPort);
            if (!phiData.empty()) { phi = phiData[0]; phiFromPort = true; }
        }

        if (!thetaFromPort && std::fabs(theta) > kTwoPi) theta = deg2rad(theta);
        if (!phiFromPort   && std::fabs(phi)   > kTwoPi) phi   = deg2rad(phi);

        // ---- 波束合成 ----
        const double sTh = std::sin(theta);
        const double ux  = sTh * std::cos(phi);
        const double uy  = sTh * std::sin(phi);

        std::complex<double> ysum(0.0, 0.0);
        const int nUse = std::min(m_nChExpected, static_cast<int>(m_inputBuffer.size()));

        for (int idx = 0; idx < nUse; ++idx)
        {
            const int n = idx / m_nx;
            const int m = idx % m_nx;
            const double psi = kTwoPi * (m_xPos[m] * ux + m_yPos[n] * uy);
            const std::complex<double> phase(std::cos(psi), -std::sin(psi));
            ysum += m_inputBuffer[idx] * (m_taper2d[idx] * phase);
        }

        m_outputQueue.push(ysum);
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

bool RADAR_Rx_DBS_2D_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Rx_DBS_2D>();

    // 解析参数
    try { m_NumOfAntx = std::stoi(getParameter("NumOfAntx").Value); } catch (...) {}
    try { m_NumOfAnty = std::stoi(getParameter("NumOfAnty").Value); } catch (...) {}
    try { m_Dx = std::stod(getParameter("Dx").Value); } catch (...) {}
    try { m_Dy = std::stod(getParameter("Dy").Value); } catch (...) {}
    try { m_Theta = std::stod(getParameter("Theta").Value); } catch (...) {}
    try { m_Phi = std::stod(getParameter("Phi").Value); } catch (...) {}
    try { m_Window_Type = ConvertStringToWindowType(getParameter("Window_Type").Value); } catch (...) {}
    try { m_WindowParameters = std::stod(getParameter("WindowParameters").Value); } catch (...) {}

    if (m_NumOfAntx < 1 || m_NumOfAnty < 1)
    {
        LOG_ERROR("RADAR_Rx_DBS_2D: NumOfAntx and NumOfAnty must be >= 1.");
        return false;
    }

    m_nx = m_NumOfAntx;
    m_ny = m_NumOfAnty;
    m_nChExpected = m_nx * m_ny;

    rebuildCache();

    AddInputPort("input",   m_algo->input,   1, Block::DataType::DCOMPLEX_BUS);
    AddInputPort("InTheta", m_algo->InTheta, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("InPhi",   m_algo->InPhi,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_algo->output,  1, Block::DataType::DCOMPLEX_BUS);

    return true;
}

RADAR_Rx_DBS_2D::Window_TypeEnum RADAR_Rx_DBS_2D_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle"      || lower == "0") return RADAR_Rx_DBS_2D::Rectangle;
    if (lower == "bartlett"       || lower == "1") return RADAR_Rx_DBS_2D::Bartlett;
    if (lower == "hanning"        || lower == "2") return RADAR_Rx_DBS_2D::Hanning;
    if (lower == "hamming"        || lower == "3") return RADAR_Rx_DBS_2D::Hamming;
    if (lower == "blackman"       || lower == "4") return RADAR_Rx_DBS_2D::Blackman;
    if (lower == "steepblackman"  || lower == "5") return RADAR_Rx_DBS_2D::SteepBlackman;
    if (lower == "kaiser"         || lower == "6") return RADAR_Rx_DBS_2D::Kaiser;
    return RADAR_Rx_DBS_2D::Rectangle;
}

// ============================================================================
// rebuildCache
// ============================================================================

void RADAR_Rx_DBS_2D_Block::rebuildCache()
{
    // 阵元位置（corner-origin）
    m_xPos.assign(m_nx, 0.0);
    m_yPos.assign(m_ny, 0.0);
    for (int m = 0; m < m_nx; ++m) m_xPos[m] = static_cast<double>(m) * m_Dx;
    for (int n = 0; n < m_ny; ++n) m_yPos[n] = static_cast<double>(n) * m_Dy;

    // 窗函数
    makeWindow(m_Window_Type, m_nx, m_WindowParameters, m_wx);
    makeWindow(m_Window_Type, m_ny, m_WindowParameters, m_wy);

    // 二维 taper（row-major: idx = n * Nx + m）
    m_taper2d.assign(m_nChExpected, 1.0);
    for (int n = 0; n < m_ny; ++n)
        for (int m = 0; m < m_nx; ++m)
            m_taper2d[n * m_nx + m] = m_wx[m] * m_wy[n];
}

// ============================================================================
// deg2rad
// ============================================================================

double RADAR_Rx_DBS_2D_Block::deg2rad(double deg)
{
    return deg * kPi / 180.0;
}

// ============================================================================
// Kaiser I0 Bessel
// ============================================================================

double RADAR_Rx_DBS_2D_Block::i0Bessel(double x)
{
    static const double i0A[] = {
        -4.41534164647933937950E-18,  3.33079451882223809783E-17,
        -2.43127984654795469359E-16,  1.71539128555513303061E-15,
        -1.16853328779934516808E-14,  7.67618549860493561688E-14,
        -4.85644678311192946090E-13,  2.95505266312963983461E-12,
        -1.72682629144155570723E-11,  9.67580903537323691224E-11,
        -5.18979560163526290666E-10,  2.65982372468238665035E-9,
        -1.30002500998624804212E-8,   6.04699502254191894932E-8,
        -2.67079385394061173391E-7,   1.11738753912010371815E-6,
        -4.41673835845875056359E-6,   1.64484480707288970893E-5,
        -5.75419501008210370398E-5,   1.88502885095841655729E-4,
        -5.76375574538582365885E-4,   1.63947561694133579842E-3,
        -4.32430999505057594430E-3,   1.05464603945949983183E-2,
        -2.37374148058994688156E-2,   4.93052842396707084878E-2,
        -9.49010970480476444210E-2,   1.71620901522208775349E-1,
        -3.04682672343198398683E-1,   6.76795274409476084995E-1
    };
    static const double i0B[] = {
        -7.23318048787475395456E-18, -4.83050448594418207126E-18,
         4.46562142029675999901E-17,  3.46122286769746109310E-17,
        -2.82762398051658348494E-16, -3.42548561967721913462E-16,
         1.77256013305652638360E-15,  3.81168066935262242075E-15,
        -9.55484669882830764870E-15, -4.15056934728722208663E-14,
         1.54008621752140982691E-14,  3.85277838274214270114E-13,
         7.18012445138366623367E-13, -1.79417853150680611778E-12,
        -1.32158118404477131188E-11, -3.14991652796324136454E-11,
         1.18891471078464383424E-11,  4.94060238822496958910E-10,
         3.39623202570838634515E-9,   2.26666899049817806459E-8,
         2.04891858946906374183E-7,   2.89137052083475648297E-6,
         6.88975834691682398426E-5,   3.36911647825569408990E-3,
         8.04490411014108831608E-1
    };

    auto chbevl = [](double xx, const double* coef, int n) -> double {
        double b0 = coef[0], b1 = 0.0, b2 = 0.0;
        for (int i = 1; i < n; ++i) {
            b2 = b1;
            b1 = b0;
            b0 = xx * b1 - b2 + coef[i];
        }
        return 0.5 * (b0 - b2);
    };

    const double ax = std::fabs(x);
    if (ax <= 8.0) {
        const double y = chbevl(ax / 2.0 - 2.0, i0A, static_cast<int>(sizeof(i0A) / sizeof(i0A[0])));
        return std::exp(ax) * y;
    } else {
        const double y = chbevl(32.0 / ax - 2.0, i0B, static_cast<int>(sizeof(i0B) / sizeof(i0B[0])));
        return std::exp(ax) * y / std::sqrt(ax);
    }
}

// ============================================================================
// makeWindow
// ============================================================================

void RADAR_Rx_DBS_2D_Block::makeWindow(RADAR_Rx_DBS_2D::Window_TypeEnum type, int L, double beta, std::vector<double>& w)
{
    w.assign(std::max(L, 1), 1.0);
    if (L <= 1) { w[0] = 1.0; return; }

    auto omega = [L](int p) -> double {
        return kTwoPi * static_cast<double>(p) / static_cast<double>(L - 1);
    };

    switch (type)
    {
    case RADAR_Rx_DBS_2D::Rectangle:
        for (int p = 0; p < L; ++p) w[p] = 1.0;
        break;

    case RADAR_Rx_DBS_2D::Bartlett:
        for (int p = 0; p < L; ++p) {
            const double mid = 0.5 * (L - 1);
            double val = 1.0 - std::fabs((p - mid) / mid);
            if (val < 0.0) val = 0.0;
            w[p] = val;
        }
        break;

    case RADAR_Rx_DBS_2D::Hanning:
        for (int p = 0; p < L; ++p) {
            const double th = omega(p);
            w[p] = 0.5 * (1.0 - std::cos(th));
        }
        break;

    case RADAR_Rx_DBS_2D::Hamming:
        for (int p = 0; p < L; ++p) {
            const double th = omega(p);
            w[p] = 0.54 - 0.46 * std::cos(th);
        }
        break;

    case RADAR_Rx_DBS_2D::Blackman:
        for (int p = 0; p < L; ++p) {
            const double th = omega(p);
            w[p] = 0.42 - 0.5 * std::cos(th) + 0.08 * std::cos(2.0 * th);
        }
        break;

    case RADAR_Rx_DBS_2D::Kaiser: {
        const double b = std::max(beta, 0.0);
        const double denom = i0Bessel(b);
        for (int p = 0; p < L; ++p) {
            const double t = 2.0 * p / static_cast<double>(L - 1) - 1.0;
            w[p] = i0Bessel(b * std::sqrt(std::max(0.0, 1.0 - t * t))) / denom;
        }
    } break;

    case RADAR_Rx_DBS_2D::SteepBlackman: {
        const double a0 = 0.35875;
        const double a1 = 0.48829;
        const double a2 = 0.14128;
        const double a3 = 0.01168;
        const double N = static_cast<double>(L - 1);

        for (int p = 0; p < L; ++p) {
            const int n = (p < (L / 2)) ? p : (L - p - 1);
            const double th = 2.0 * kPi * static_cast<double>(n) / N;
            w[p] = a0 - a1 * std::cos(th) + a2 * std::cos(2.0 * th) - a3 * std::cos(3.0 * th);
        }
    } break;

    default:
        for (int p = 0; p < L; ++p) w[p] = 1.0;
        break;
    }
}
