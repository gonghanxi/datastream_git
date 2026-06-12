#include "RADAR_Kalman_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <string>

// ============================================================================
// 常量
// ============================================================================

const double RADAR_Kalman_Block::kTiny     = 1.0e-12;
const double RADAR_Kalman_Block::kProbTiny = 1.0e-300;
const double RADAR_Kalman_Block::kPi       = 3.1415926535897932384626433832795;

// ============================================================================
// 匿名命名空间：纯静态工具函数（不访问 Block 成员）
// ============================================================================

namespace {

// ---- 字符串工具 ----
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

// ---- 矩阵解析 ----
SystemVueModelBuilder::DoubleMatrix ParseStringToDoubleMatrix(const std::string& value)
{
    SystemVueModelBuilder::DoubleMatrix mat;
    std::string s = TrimCopy(value);
    if (s.empty()) return mat;

    if (s.front() == '[' && s.back() == ']')
        s = s.substr(1, s.size() - 2);

    std::vector<std::vector<double>> rows;
    std::string::size_type pos = 0;
    while (pos < s.size()) {
        auto semi = s.find(';', pos);
        std::string rowStr = (semi == std::string::npos) ? s.substr(pos) : s.substr(pos, semi - pos);
        rowStr = TrimCopy(rowStr);

        std::vector<double> row;
        std::istringstream iss(rowStr);
        std::string token;
        while (std::getline(iss, token, ' ')) {
            token = TrimCopy(token);
            if (!token.empty()) {
                try { row.push_back(std::stod(token)); } catch (...) {}
            }
        }
        if (!row.empty()) rows.push_back(row);

        if (semi == std::string::npos) break;
        pos = semi + 1;
    }

    if (rows.empty()) return mat;
    size_t nCols = rows[0].size();
    mat.Resize(rows.size(), nCols);
    for (size_t r = 0; r < rows.size(); ++r)
        for (size_t c = 0; c < nCols && c < rows[r].size(); ++c)
            mat(r, c) = rows[r][c];

    return mat;
}

void MatrixToColumnMajor(const SystemVueModelBuilder::DoubleMatrix& mat,
                         std::vector<double>& arr, int& arrSize)
{
    const size_t nRows = mat.NumRows();
    const size_t nCols = mat.NumColumns();
    arrSize = static_cast<int>(nRows * nCols);
    arr.resize(arrSize);
    for (size_t c = 0; c < nCols; ++c)
        for (size_t r = 0; r < nRows; ++r)
            arr[r + c * nRows] = mat(r, c);
}

// ---- 移植自 RADAR_Kalman::get_array_value_ / get_mat_value_ ----
double get_array_value(const double* p, int size, int idx, double defval)
{
    if (p == nullptr || idx < 0 || idx >= size)
        return defval;
    return p[idx];
}

double get_mat_value(const double* p, int size, int rows, int cols,
                     int r, int c, const double* defval)
{
    const int dstRowMajor = r * cols + c;
    const int srcColumnMajor = c * rows + r;
    return get_array_value(p, size, srcColumnMajor, defval[dstRowMajor]);
}

// ---- 移植自 RADAR_Kalman 数值工具 ----
void normalize_mu(double mu[3])
{
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(mu[i]) || mu[i] < 0.0)
            mu[i] = 0.0;
    }
    double s = mu[0] + mu[1] + mu[2];
    if (s <= 1.0e-12) {
        mu[0] = 1.0; mu[1] = 0.0; mu[2] = 0.0;
        return;
    }
    mu[0] /= s; mu[1] /= s; mu[2] /= s;
}

void sym2(double P[2][2])
{
    const double a = 0.5 * (P[0][1] + P[1][0]);
    P[0][1] = a; P[1][0] = a;
}

void sym3(double P[3][3])
{
    for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j) {
            const double a = 0.5 * (P[i][j] + P[j][i]);
            P[i][j] = a; P[j][i] = a;
        }
}

void positive_guard2(double P[2][2])
{
    sym2(P);
    for (int i = 0; i < 2; ++i) {
        if (!std::isfinite(P[i][i]) || P[i][i] < 1.0e-12)
            P[i][i] = 1.0e-12;
    }
}

void positive_guard3(double P[3][3])
{
    sym3(P);
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(P[i][i]) || P[i][i] < 1.0e-12)
            P[i][i] = 1.0e-12;
    }
}

// ---- 移植自 RADAR_Kalman::predict_update_cv_ ----
void predict_update_cv(double x[2], double P[2][2],
                       const double A[2][2], const double H[2], const double G[2],
                       double q, double R, double z, double& likelihood)
{
    double xp[2];
    xp[0] = A[0][0] * x[0] + A[0][1] * x[1];
    xp[1] = A[1][0] * x[0] + A[1][1] * x[1];

    double AP[2][2];
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            AP[r][c] = A[r][0] * P[0][c] + A[r][1] * P[1][c];

    double Pp[2][2];
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            Pp[r][c] = AP[r][0] * A[c][0] + AP[r][1] * A[c][1] + q * G[r] * G[c];
    positive_guard2(Pp);

    const double zPred = H[0] * xp[0] + H[1] * xp[1];
    const double y = z - zPred;

    const double PH0 = Pp[0][0] * H[0] + Pp[0][1] * H[1];
    const double PH1 = Pp[1][0] * H[0] + Pp[1][1] * H[1];
    double S = H[0] * PH0 + H[1] * PH1 + R;
    if (!std::isfinite(S) || S < 1.0e-12) S = 1.0e-12;

    const double K0 = PH0 / S;
    const double K1 = PH1 / S;

    x[0] = xp[0] + K0 * y;
    x[1] = xp[1] + K1 * y;

    double M[2][2];
    M[0][0] = 1.0 - K0 * H[0];  M[0][1] = -K0 * H[1];
    M[1][0] = -K1 * H[0];       M[1][1] = 1.0 - K1 * H[1];

    double MP[2][2];
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            MP[r][c] = M[r][0] * Pp[0][c] + M[r][1] * Pp[1][c];

    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            P[r][c] = MP[r][0] * M[c][0] + MP[r][1] * M[c][1]
                    + R * ((r == 0) ? K0 : K1) * ((c == 0) ? K0 : K1);
    positive_guard2(P);

    const double exponent = -0.5 * y * y / S;
    likelihood = (exponent < -690.0) ? 1.0e-300
                                     : std::exp(exponent) / std::sqrt(std::max(S, 1.0e-12));
    if (!std::isfinite(likelihood) || likelihood < 1.0e-300)
        likelihood = 1.0e-300;
}

// ---- 移植自 RADAR_Kalman::predict_update_ca_ ----
void predict_update_ca(double x[3], double P[3][3],
                       const double A[3][3], const double H[3], const double G[3],
                       double q, double R, double z, double& likelihood)
{
    double xp[3];
    for (int r = 0; r < 3; ++r)
        xp[r] = A[r][0] * x[0] + A[r][1] * x[1] + A[r][2] * x[2];

    double AP[3][3];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            AP[r][c] = A[r][0] * P[0][c] + A[r][1] * P[1][c] + A[r][2] * P[2][c];

    double Pp[3][3];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            Pp[r][c] = AP[r][0] * A[c][0] + AP[r][1] * A[c][1] + AP[r][2] * A[c][2]
                     + q * G[r] * G[c];
    positive_guard3(Pp);

    const double zPred = H[0] * xp[0] + H[1] * xp[1] + H[2] * xp[2];
    const double y = z - zPred;

    double PH[3];
    for (int r = 0; r < 3; ++r)
        PH[r] = Pp[r][0] * H[0] + Pp[r][1] * H[1] + Pp[r][2] * H[2];

    double S = H[0] * PH[0] + H[1] * PH[1] + H[2] * PH[2] + R;
    if (!std::isfinite(S) || S < 1.0e-12) S = 1.0e-12;

    double K[3];
    for (int r = 0; r < 3; ++r) K[r] = PH[r] / S;
    for (int r = 0; r < 3; ++r) x[r] = xp[r] + K[r] * y;

    double M[3][3];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            M[r][c] = ((r == c) ? 1.0 : 0.0) - K[r] * H[c];

    double MP[3][3];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            MP[r][c] = M[r][0] * Pp[0][c] + M[r][1] * Pp[1][c] + M[r][2] * Pp[2][c];

    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            P[r][c] = MP[r][0] * M[c][0] + MP[r][1] * M[c][1] + MP[r][2] * M[c][2]
                    + R * K[r] * K[c];
    positive_guard3(P);

    const double exponent = -0.5 * y * y / S;
    likelihood = (exponent < -690.0) ? 1.0e-300
                                     : std::exp(exponent) / std::sqrt(std::max(S, 1.0e-12));
    if (!std::isfinite(likelihood) || likelihood < 1.0e-300)
        likelihood = 1.0e-300;
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Kalman_Block::RADAR_Kalman_Block(const std::string& name)
    : Block(name)
    , m_Period(2.0)
    , m_Meas_err_var(10000.0)
    , m_r_matSize(0), m_a1_matSize(0), m_h1_matSize(0), m_g1_matSize(0)
    , m_a2_matSize(0), m_h2_matSize(0), m_g2_matSize(0)
    , m_q1_matSize(0), m_q2_matSize(0), m_q3_matSize(0)
    , m_p_matSize(0), m_u_matSize(0)
    , m_sampleIndex(0ULL)
{
    reset_axis_(m_axisX);
    reset_axis_(m_axisY);
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Kalman_Block::SetDefaultParameters()
{
    m_Period       = 2.0;
    m_Meas_err_var = 10000.0;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Kalman_Block::Setup()
{
    Block::Setup();

    m_inputBufferX.clear();
    m_inputBufferY.clear();
    while (!m_outputQueueX.empty()) m_outputQueueX.pop();
    while (!m_outputQueueY.empty()) m_outputQueueY.pop();

    m_sampleIndex = 0ULL;
    reset_axis_(m_axisX);
    reset_axis_(m_axisY);

    double u0[3];
    get_u0_(u0);
    for (int i = 0; i < 3; ++i) {
        m_axisX.mu[i] = u0[i];
        m_axisY.mu[i] = u0[i];
    }

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_Kalman_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式（内联自 RADAR_Kalman::Run）
// ============================================================================

bool RADAR_Kalman_Block::DataStreamRun()
{
    auto xData = ReadInputData<double>(GetInputPortName(0));
    auto yData = ReadInputData<double>(GetInputPortName(1));
    if (xData.empty() || yData.empty()) return true;

    const double zx = xData[0];
    const double zy = yData[0];
    double xEst = zx;
    double yEst = zy;

    if (m_sampleIndex == 0ULL) {
        init_axis_from_first_sample_(m_axisX, zx);
        init_axis_from_first_sample_(m_axisY, zy);
    } else if (m_sampleIndex == 1ULL) {
        init_axis_from_second_sample_(m_axisX, zx);
        init_axis_from_second_sample_(m_axisY, zy);
    } else {
        xEst = process_axis_(m_axisX, zx, 0);
        yEst = process_axis_(m_axisY, zy, 1);
    }

    ++m_sampleIndex;

    std::vector<double> xVec; xVec.push_back(xEst);
    std::vector<double> yVec; yVec.push_back(yEst);
    WriteOutputData(GetOutputPortName(0), xVec);
    WriteOutputData(GetOutputPortName(1), yVec);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式（内联自 RADAR_Kalman::Run）
// ============================================================================

bool RADAR_Kalman_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto xData = ReadInputData<double>(GetInputPortName(0));
        auto yData = ReadInputData<double>(GetInputPortName(1));
        if (xData.empty() || yData.empty()) return true;
        m_inputBufferX.push_back(xData[0]);
        m_inputBufferY.push_back(yData[0]);
    }

    // ② 判断阈值（rate=1），处理
    if (!m_inputBufferX.empty() && !m_inputBufferY.empty()) {
        const double zx = m_inputBufferX.front();
        const double zy = m_inputBufferY.front();
        double xEst = zx;
        double yEst = zy;

        if (m_sampleIndex == 0ULL) {
            init_axis_from_first_sample_(m_axisX, zx);
            init_axis_from_first_sample_(m_axisY, zy);
        } else if (m_sampleIndex == 1ULL) {
            init_axis_from_second_sample_(m_axisX, zx);
            init_axis_from_second_sample_(m_axisY, zy);
        } else {
            xEst = process_axis_(m_axisX, zx, 0);
            yEst = process_axis_(m_axisY, zy, 1);
        }

        ++m_sampleIndex;

        m_outputQueueX.push(xEst);
        m_outputQueueY.push(yEst);

        m_inputBufferX.clear();
        m_inputBufferY.clear();
    }

    // ③ 出队写入
    if (!m_outputQueueX.empty()) {
        std::vector<double> xVec; xVec.push_back(m_outputQueueX.front());
        WriteOutputData(GetOutputPortName(0), xVec);
        m_outputQueueX.pop();
    }

    if (!m_outputQueueY.empty()) {
        std::vector<double> yVec; yVec.push_back(m_outputQueueY.front());
        WriteOutputData(GetOutputPortName(1), yVec);
        m_outputQueueY.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Kalman_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Kalman>();

    SetDefaultParameters();

    try { m_Period       = std::stod(getParameter("Period").Value); }       catch (...) { LOG_WARN("Failed to parse parameter 'Period', using default value."); }
    try { m_Meas_err_var = std::stod(getParameter("Meas_err_var").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Meas_err_var', using default value."); }

    // 解析矩阵参数
    SystemVueModelBuilder::DoubleMatrix r_mat, a1_mat, h1_mat, g1_mat;
    SystemVueModelBuilder::DoubleMatrix a2_mat, h2_mat, g2_mat;
    SystemVueModelBuilder::DoubleMatrix q1_mat, q2_mat, q3_mat, p_mat, u_mat;

    try { r_mat  = ParseStringToDoubleMatrix(getParameter("r_mat").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'r_mat', using default value."); }
    try { a1_mat = ParseStringToDoubleMatrix(getParameter("a1_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'a1_mat', using default value."); }
    try { h1_mat = ParseStringToDoubleMatrix(getParameter("h1_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'h1_mat', using default value."); }
    try { g1_mat = ParseStringToDoubleMatrix(getParameter("g1_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'g1_mat', using default value."); }
    try { a2_mat = ParseStringToDoubleMatrix(getParameter("a2_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'a2_mat', using default value."); }
    try { h2_mat = ParseStringToDoubleMatrix(getParameter("h2_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'h2_mat', using default value."); }
    try { g2_mat = ParseStringToDoubleMatrix(getParameter("g2_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'g2_mat', using default value."); }
    try { q1_mat = ParseStringToDoubleMatrix(getParameter("q1_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'q1_mat', using default value."); }
    try { q2_mat = ParseStringToDoubleMatrix(getParameter("q2_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'q2_mat', using default value."); }
    try { q3_mat = ParseStringToDoubleMatrix(getParameter("q3_mat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'q3_mat', using default value."); }
    try { p_mat  = ParseStringToDoubleMatrix(getParameter("p_mat").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'p_mat', using default value."); }
    try { u_mat  = ParseStringToDoubleMatrix(getParameter("u_mat").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'u_mat', using default value."); }

    MatrixToColumnMajor(r_mat,  m_r_arr,  m_r_matSize);
    MatrixToColumnMajor(a1_mat, m_a1_arr, m_a1_matSize);
    MatrixToColumnMajor(h1_mat, m_h1_arr, m_h1_matSize);
    MatrixToColumnMajor(g1_mat, m_g1_arr, m_g1_matSize);
    MatrixToColumnMajor(a2_mat, m_a2_arr, m_a2_matSize);
    MatrixToColumnMajor(h2_mat, m_h2_arr, m_h2_matSize);
    MatrixToColumnMajor(g2_mat, m_g2_arr, m_g2_matSize);
    MatrixToColumnMajor(q1_mat, m_q1_arr, m_q1_matSize);
    MatrixToColumnMajor(q2_mat, m_q2_arr, m_q2_matSize);
    MatrixToColumnMajor(q3_mat, m_q3_arr, m_q3_matSize);
    MatrixToColumnMajor(p_mat,  m_p_arr,  m_p_matSize);
    MatrixToColumnMajor(u_mat,  m_u_arr,  m_u_matSize);

    // 端口注册（通过 m_algo 端口成员绑定）
    AddInputPort("x_in",  m_algo->x_in,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("y_in",  m_algo->y_in,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("x_out", m_algo->x_out, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("y_out", m_algo->y_out, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 移植自 RADAR_Kalman::get_R_axis_
// ============================================================================

double RADAR_Kalman_Block::get_R_axis_(int axis) const
{
    static const double dR[] = { 10000.0, 0.0, 0.0, 10000.0 };
    double v = get_mat_value(m_r_arr.data(), m_r_matSize, 2, 2, axis, axis, dR);
    if (!std::isfinite(v) || v <= 0.0)
        v = (m_Meas_err_var > 0.0 && std::isfinite(m_Meas_err_var)) ? m_Meas_err_var : 10000.0;
    return v;
}

// ============================================================================
// 移植自 RADAR_Kalman::get_Pmarkov_
// ============================================================================

double RADAR_Kalman_Block::get_Pmarkov_(int r, int c) const
{
    static const double dP[] = {
        0.95, 0.025, 0.025,
        0.025, 0.95, 0.025,
        0.025, 0.025, 0.95
    };
    double v = get_mat_value(m_p_arr.data(), m_p_matSize, 3, 3, r, c, dP);
    if (!std::isfinite(v) || v < 0.0) v = 0.0;
    return v;
}

// ============================================================================
// 移植自 RADAR_Kalman::get_u0_
// ============================================================================

void RADAR_Kalman_Block::get_u0_(double u[3]) const
{
    u[0] = get_array_value(m_u_arr.data(), m_u_matSize, 0, 1.0);
    u[1] = get_array_value(m_u_arr.data(), m_u_matSize, 1, 0.0);
    u[2] = get_array_value(m_u_arr.data(), m_u_matSize, 2, 0.0);
    normalize_mu(u);
}

// ============================================================================
// 移植自 RADAR_Kalman::extract_cv_matrices_
// ============================================================================

void RADAR_Kalman_Block::extract_cv_matrices_(int axis,
    double A[2][2], double H[2], double G[2], double& q, double& R) const
{
    static const double dA1[] = { 1,2,0,0, 0,1,0,0, 0,0,1,2, 0,0,0,1 };
    static const double dH1[] = { 1,0,0,0, 0,0,1,0 };
    static const double dG1[] = { 1,0, 1,0, 0,1, 0,1 };
    static const double dQ1[] = { 0,0,0,0 };

    const int s0 = (axis == 0) ? 0 : 2;
    const int s1 = (axis == 0) ? 1 : 3;
    const int idx[2] = { s0, s1 };

    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            A[r][c] = get_mat_value(m_a1_arr.data(), m_a1_matSize, 4, 4, idx[r], idx[c], dA1);

    for (int c = 0; c < 2; ++c)
        H[c] = get_mat_value(m_h1_arr.data(), m_h1_matSize, 2, 4, axis, idx[c], dH1);

    for (int r = 0; r < 2; ++r)
        G[r] = get_mat_value(m_g1_arr.data(), m_g1_matSize, 4, 2, idx[r], axis, dG1);

    q = get_mat_value(m_q1_arr.data(), m_q1_matSize, 2, 2, axis, axis, dQ1);
    if (!std::isfinite(q) || q < 0.0) q = 0.0;

    R = get_R_axis_(axis);
}

// ============================================================================
// 移植自 RADAR_Kalman::extract_ca_matrices_
// ============================================================================

void RADAR_Kalman_Block::extract_ca_matrices_(int axis,
    const double* qPtr, int qSize,
    double A[3][3], double H[3], double G[3], double& q, double& R) const
{
    static const double dA2[] = { 1,1,0,0,2,0, 0,1,0,0,2,0, 0,0,1,2,0,2, 0,0,0,1,0,2, 0,0,0,0,1,0, 0,0,0,0,0,1 };
    static const double dH2[] = { 1,0,0,0,0,0, 0,0,1,0,0,0 };
    static const double dG2[] = { 1,0, 2,0, 0,1, 0,2, 1,0, 0,1 };
    static const double dQ2[] = { 0.05,0,0,0.05 };
    static const double dQ3[] = { 0.02,0,0,0.02 };

    const int idx[3] = { (axis == 0) ? 0 : 2, (axis == 0) ? 1 : 3, (axis == 0) ? 4 : 5 };

    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            A[r][c] = get_mat_value(m_a2_arr.data(), m_a2_matSize, 6, 6, idx[r], idx[c], dA2);

    for (int c = 0; c < 3; ++c)
        H[c] = get_mat_value(m_h2_arr.data(), m_h2_matSize, 2, 6, axis, idx[c], dH2);

    for (int r = 0; r < 3; ++r)
        G[r] = get_mat_value(m_g2_arr.data(), m_g2_matSize, 6, 2, idx[r], axis, dG2);

    const double* dq = (qPtr == m_q2_arr.data()) ? dQ2 : dQ3;
    q = get_mat_value(qPtr, qSize, 2, 2, axis, axis, dq);
    if (!std::isfinite(q) || q < 0.0) q = 0.0;

    R = get_R_axis_(axis);
}

// ============================================================================
// 移植自 RADAR_Kalman::reset_axis_
// ============================================================================

void RADAR_Kalman_Block::reset_axis_(AxisIMM& f) const
{
    f.hasFirst = false;
    f.zPrev = 0.0;
    f.mu[0] = 1.0; f.mu[1] = 0.0; f.mu[2] = 0.0;

    for (int i = 0; i < 2; ++i) {
        f.x1[i] = 0.0;
        for (int j = 0; j < 2; ++j) f.P1[i][j] = 0.0;
    }
    for (int i = 0; i < 3; ++i) {
        f.x2[i] = 0.0; f.x3[i] = 0.0;
        for (int j = 0; j < 3; ++j) { f.P2[i][j] = 0.0; f.P3[i][j] = 0.0; }
    }
}

// ============================================================================
// 移植自 RADAR_Kalman::init_axis_from_first_sample_
// ============================================================================

void RADAR_Kalman_Block::init_axis_from_first_sample_(AxisIMM& f, double z) const
{
    double u[3];
    get_u0_(u);
    f.mu[0] = u[0]; f.mu[1] = u[1]; f.mu[2] = u[2];

    const double Rscale = (m_Meas_err_var > 0.0 && std::isfinite(m_Meas_err_var)) ? m_Meas_err_var : 10000.0;
    const double Ppos = 4.0 * Rscale;
    const double Pvel = 1.0 * Rscale;
    const double Pacc = 0.25 * Rscale;

    f.x1[0] = z; f.x1[1] = 0.0;
    f.P1[0][0] = Ppos; f.P1[0][1] = 0.0; f.P1[1][0] = 0.0; f.P1[1][1] = Pvel;

    f.x2[0] = z; f.x2[1] = 0.0; f.x2[2] = 0.0;
    f.x3[0] = z; f.x3[1] = 0.0; f.x3[2] = 0.0;

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) { f.P2[i][j] = 0.0; f.P3[i][j] = 0.0; }

    f.P2[0][0] = f.P3[0][0] = Ppos;
    f.P2[1][1] = f.P3[1][1] = Pvel;
    f.P2[2][2] = f.P3[2][2] = Pacc;

    f.zPrev = z;
    f.hasFirst = true;
}

// ============================================================================
// 移植自 RADAR_Kalman::init_axis_from_second_sample_
// ============================================================================

void RADAR_Kalman_Block::init_axis_from_second_sample_(AxisIMM& f, double z) const
{
    const double T = (m_Period > kTiny && std::isfinite(m_Period)) ? m_Period : 2.0;
    const double v = (z - f.zPrev) / T;

    f.x1[0] = z; f.x1[1] = v;
    f.x2[0] = z; f.x2[1] = v; f.x2[2] = 0.0;
    f.x3[0] = z; f.x3[1] = v; f.x3[2] = 0.0;
    f.zPrev = z;
}

// ============================================================================
// 移植自 RADAR_Kalman::mix_axis_
// ============================================================================

void RADAR_Kalman_Block::mix_axis_(const AxisIMM& f,
    double c[3],
    double mixX1[2], double mixP1[2][2],
    double mixX2[3], double mixP2[3][3],
    double mixX3[3], double mixP3[3][3]) const
{
    const double* xs[3] = { f.x1, f.x2, f.x3 };
    const int dims[3] = { 2, 3, 3 };

    double Pmark[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            Pmark[i][j] = get_Pmarkov_(i, j);

    double mixProb[3][3];
    for (int j = 0; j < 3; ++j) {
        c[j] = 0.0;
        for (int i = 0; i < 3; ++i)
            c[j] += Pmark[i][j] * f.mu[i];
        if (!std::isfinite(c[j]) || c[j] <= kProbTiny) c[j] = kProbTiny;

        for (int i = 0; i < 3; ++i)
            mixProb[i][j] = Pmark[i][j] * f.mu[i] / c[j];
    }

    // ---- 目标模型 1: CV 维度 2 ----
    for (int r = 0; r < 2; ++r) {
        mixX1[r] = 0.0;
        for (int c0 = 0; c0 < 2; ++c0) mixP1[r][c0] = 0.0;
    }
    for (int i = 0; i < 3; ++i) {
        mixX1[0] += mixProb[i][0] * xs[i][0];
        mixX1[1] += mixProb[i][0] * xs[i][1];
    }
    for (int i = 0; i < 3; ++i) {
        double Pi2[2][2];
        if (dims[i] == 2) {
            Pi2[0][0] = f.P1[0][0]; Pi2[0][1] = f.P1[0][1];
            Pi2[1][0] = f.P1[1][0]; Pi2[1][1] = f.P1[1][1];
        } else if (i == 1) {
            Pi2[0][0] = f.P2[0][0]; Pi2[0][1] = f.P2[0][1];
            Pi2[1][0] = f.P2[1][0]; Pi2[1][1] = f.P2[1][1];
        } else {
            Pi2[0][0] = f.P3[0][0]; Pi2[0][1] = f.P3[0][1];
            Pi2[1][0] = f.P3[1][0]; Pi2[1][1] = f.P3[1][1];
        }

        for (int r = 0; r < 2; ++r)
            for (int c0 = 0; c0 < 2; ++c0) {
                const double dr = xs[i][r] - mixX1[r];
                const double dc = xs[i][c0] - mixX1[c0];
                mixP1[r][c0] += mixProb[i][0] * (Pi2[r][c0] + dr * dc);
            }
    }
    positive_guard2(mixP1);

    // ---- 目标模型 2/3: CA 维度 3 ----
    double* mixX[2] = { mixX2, mixX3 };
    double(*mixP[2])[3] = { mixP2, mixP3 };

    for (int dest = 0; dest < 2; ++dest) {
        const int jModel = dest + 1;
        for (int r = 0; r < 3; ++r) {
            mixX[dest][r] = 0.0;
            for (int c0 = 0; c0 < 3; ++c0) mixP[dest][r][c0] = 0.0;
        }

        for (int i = 0; i < 3; ++i) {
            mixX[dest][0] += mixProb[i][jModel] * xs[i][0];
            mixX[dest][1] += mixProb[i][jModel] * xs[i][1];
            mixX[dest][2] += mixProb[i][jModel] * ((dims[i] == 3) ? xs[i][2] : 0.0);
        }

        for (int i = 0; i < 3; ++i) {
            double Pi3[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
            double xi3[3] = { xs[i][0], xs[i][1], (dims[i] == 3) ? xs[i][2] : 0.0 };
            if (dims[i] == 2) {
                Pi3[0][0] = f.P1[0][0]; Pi3[0][1] = f.P1[0][1];
                Pi3[1][0] = f.P1[1][0]; Pi3[1][1] = f.P1[1][1];
                Pi3[2][2] = (m_Meas_err_var > 0.0) ? 0.25 * m_Meas_err_var : 2500.0;
            } else if (i == 1) {
                for (int r = 0; r < 3; ++r)
                    for (int c0 = 0; c0 < 3; ++c0) Pi3[r][c0] = f.P2[r][c0];
            } else {
                for (int r = 0; r < 3; ++r)
                    for (int c0 = 0; c0 < 3; ++c0) Pi3[r][c0] = f.P3[r][c0];
            }

            for (int r = 0; r < 3; ++r)
                for (int c0 = 0; c0 < 3; ++c0) {
                    const double dr = xi3[r] - mixX[dest][r];
                    const double dc = xi3[c0] - mixX[dest][c0];
                    mixP[dest][r][c0] += mixProb[i][jModel] * (Pi3[r][c0] + dr * dc);
                }
        }
        positive_guard3(mixP[dest]);
    }
}

// ============================================================================
// 移植自 RADAR_Kalman::process_axis_
// ============================================================================

double RADAR_Kalman_Block::process_axis_(AxisIMM& f, double z, int axis)
{
    double c[3];
    double mixX1[2], mixP1[2][2];
    double mixX2[3], mixP2[3][3];
    double mixX3[3], mixP3[3][3];

    mix_axis_(f, c, mixX1, mixP1, mixX2, mixP2, mixX3, mixP3);

    double A1[2][2], H1[2], G1[2], q1, R1;
    double A2[3][3], H2[3], G2[3], q2, R2;
    double A3[3][3], H3[3], G3[3], q3, R3;

    extract_cv_matrices_(axis, A1, H1, G1, q1, R1);
    extract_ca_matrices_(axis, m_q2_arr.data(), m_q2_matSize, A2, H2, G2, q2, R2);
    extract_ca_matrices_(axis, m_q3_arr.data(), m_q3_matSize, A3, H3, G3, q3, R3);

    double like[3];
    predict_update_cv(mixX1, mixP1, A1, H1, G1, q1, R1, z, like[0]);
    predict_update_ca(mixX2, mixP2, A2, H2, G2, q2, R2, z, like[1]);
    predict_update_ca(mixX3, mixP3, A3, H3, G3, q3, R3, z, like[2]);

    double newMu[3];
    double sumMu = 0.0;
    for (int j = 0; j < 3; ++j) {
        newMu[j] = std::max(like[j], kProbTiny) * std::max(c[j], kTiny);
        if (!std::isfinite(newMu[j]) || newMu[j] < 0.0) newMu[j] = 0.0;
        sumMu += newMu[j];
    }

    if (sumMu <= kTiny) {
        newMu[0] = f.mu[0]; newMu[1] = f.mu[1]; newMu[2] = f.mu[2];
    }

    normalize_mu(newMu);

    for (int i = 0; i < 2; ++i) {
        f.x1[i] = mixX1[i];
        for (int j = 0; j < 2; ++j) f.P1[i][j] = mixP1[i][j];
    }

    for (int i = 0; i < 3; ++i) {
        f.x2[i] = mixX2[i]; f.x3[i] = mixX3[i];
        for (int j = 0; j < 3; ++j) {
            f.P2[i][j] = mixP2[i][j];
            f.P3[i][j] = mixP3[i][j];
        }
        f.mu[i] = newMu[i];
    }

    f.zPrev = z;
    return f.mu[0] * f.x1[0] + f.mu[1] * f.x2[0] + f.mu[2] * f.x3[0];
}
