#include "SVD_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

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

// ---------- SVD helpers ----------

using DoubleMatrix = SystemVueModelBuilder::DoubleMatrix;

inline size_t rows_of(const DoubleMatrix& m) { return m.NumRows(); }
inline size_t cols_of(const DoubleMatrix& m) { return m.NumColumns(); }

inline double SIGN(double a, double b) { return (b >= 0.0) ? std::fabs(a) : -std::fabs(a); }

static double hypot_(double a, double b)
{
    a = std::fabs(a);
    b = std::fabs(b);
    if (a > b) {
        double r = b / a;
        return a * std::sqrt(1.0 + r * r);
    }
    if (b > 0.0) {
        double r = a / b;
        return b * std::sqrt(1.0 + r * r);
    }
    return 0.0;
}

inline void swap_cols(DoubleMatrix& M, int c1, int c2)
{
    const int r = static_cast<int>(M.NumRows());
    for (int i = 0; i < r; ++i)
        std::swap(M(i, c1), M(i, c2));
}

inline void scale_col(DoubleMatrix& M, int c, double s)
{
    const int r = static_cast<int>(M.NumRows());
    for (int i = 0; i < r; ++i)
        M(i, c) *= s;
}

inline double dot_col(const DoubleMatrix& A, const DoubleMatrix& B, int c)
{
    const int r = static_cast<int>(A.NumRows());
    double acc = 0.0;
    for (int i = 0; i < r; ++i) acc += A(i, c) * B(i, c);
    return acc;
}

inline int pivot_row_maxabs(const DoubleMatrix& M, int c)
{
    const int r = static_cast<int>(M.NumRows());
    int idx = 0;
    double best = 0.0;
    for (int i = 0; i < r; ++i) {
        double v = std::fabs(M(i, c));
        if (v > best) { best = v; idx = i; }
    }
    return idx;
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

SVD_M_Block::SVD_M_Block(const std::string& name)
    : Block(name)
    , m_Threshold(1e-17)
    , m_MaxIterations(30)
    , m_GenerateLeft(SVD_M::GenerateLeft)
    , m_GenerateRight(SVD_M::GenerateRight)
    , m_hasPrevV(false)
{
}

// ============================================================================
// Setup
// ============================================================================

bool SVD_M_Block::Setup()
{
    Block::Setup();

    if (m_MaxIterations < 1)
    {
        LOG_ERROR("MaxIterations must be >= 1.");
        return false;
    }
    if (m_Threshold < 0.0)
    {
        LOG_ERROR("Threshold must be >= 0.");
        return false;
    }

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool SVD_M_Block::Run()
{
    auto inputData = ReadInputData<DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const DoubleMatrix& A = inputData[0];
    const size_t nrows = rows_of(A);
    const size_t ncols = cols_of(A);

    DoubleMatrix S;      // svals output (column vector)
    DoubleMatrix Uout, Vout;

    if (nrows == 0 || ncols == 0)
    {
        // 输出空矩阵
        std::vector<DoubleMatrix> emptyOut;
        emptyOut.push_back(DoubleMatrix());
        WriteOutputData(GetOutputPortName(0), emptyOut);
        WriteOutputData(GetOutputPortName(1), emptyOut);
        WriteOutputData(GetOutputPortName(2), emptyOut);
        return true;
    }

    const bool needLeft  = (m_GenerateLeft == SVD_M::GenerateLeft);
    const bool needRight = (m_GenerateRight == SVD_M::GenerateRight);

    if (nrows >= ncols)
    {
        // A(m×n) = U(m×n) * W(n×n) * V(n×n)^T
        DoubleMatrix U, W, V;
        calc_svd(A, U, W, V, m_Threshold, m_MaxIterations, needRight ? 1 : 0);

        // svals: diag(W) as (n×1)
        S.Resize(static_cast<int>(ncols), 1);
        for (size_t i = 0; i < ncols; ++i)
            S(static_cast<int>(i), 0) = W(static_cast<int>(i), static_cast<int>(i));

        if (needLeft)  Uout = U; else Uout.Resize(0, 0);
        if (needRight) Vout = V; else Vout.Resize(0, 0);

        // 帧间符号稳定性
        if (needRight)
        {
            if (!m_hasPrevV || m_prevV.NumRows() != Vout.NumRows() || m_prevV.NumColumns() != Vout.NumColumns())
            {
                m_prevV = Vout;
                m_hasPrevV = true;
            }
            else
            {
                const int n = static_cast<int>(Vout.NumColumns());
                for (int c = 0; c < n; ++c)
                {
                    double d = dot_col(m_prevV, Vout, c);
                    if (d < 0.0)
                    {
                        scale_col(Vout, c, -1.0);
                        if (needLeft) scale_col(Uout, c, -1.0);
                    }
                }
                m_prevV = Vout;
            }
        }
        else
        {
            m_hasPrevV = false;
            m_prevV.Resize(0, 0);
        }
    }
    else
    {
        // nrows < ncols: 转置后计算 SVD
        DoubleMatrix AT, U2, W2, V2;
        transpose(A, AT);

        calc_svd(AT, U2, W2, V2, m_Threshold, m_MaxIterations, 1);

        // svals: diag(W2) as (nrows×1)
        S.Resize(static_cast<int>(nrows), 1);
        for (size_t i = 0; i < nrows; ++i)
            S(static_cast<int>(i), 0) = W2(static_cast<int>(i), static_cast<int>(i));

        if (needLeft)
            transpose(V2, Uout);
        else
            Uout.Resize(0, 0);

        if (needRight)
            transpose(U2, Vout);
        else
            Vout.Resize(0, 0);

        m_hasPrevV = false;
        m_prevV.Resize(0, 0);
    }

    // 写入 Block 输出端口
    std::vector<DoubleMatrix> svalsOut;
    svalsOut.push_back(S);
    WriteOutputData(GetOutputPortName(0), svalsOut);

    std::vector<DoubleMatrix> lsvecOut;
    lsvecOut.push_back(Uout);
    WriteOutputData(GetOutputPortName(1), lsvecOut);

    std::vector<DoubleMatrix> rsvecOut;
    rsvecOut.push_back(Vout);
    WriteOutputData(GetOutputPortName(2), rsvecOut);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool SVD_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_SVD_M = std::make_unique<SVD_M>();

    SetDefaultParameters();

    try { m_Threshold      = std::stod(getParameter("Threshold").Value);      } catch (...) { LOG_WARN("Failed to parse parameter 'Threshold', using default value."); }
    try { m_MaxIterations  = std::stoi(getParameter("MaxIterations").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'MaxIterations', using default value."); }
    try { m_GenerateLeft   = ConvertStringToGenerateLeft(getParameter("GenerateLeft").Value);   } catch (...) { LOG_WARN("Failed to parse parameter 'GenerateLeft', using default value."); }
    try { m_GenerateRight  = ConvertStringToGenerateRight(getParameter("GenerateRight").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'GenerateRight', using default value."); }

    if (m_MaxIterations < 1) m_MaxIterations = 1;
    if (m_Threshold < 0.0)   m_Threshold = 0.0;

    m_hasPrevV = false;
    m_prevV.Resize(0, 0);

    AddInputPort("input",  m_SVD_M->input,  1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("svals", m_SVD_M->svals, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("lsvec", m_SVD_M->lsvec, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("rsvec", m_SVD_M->rsvec, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void SVD_M_Block::SetDefaultParameters()
{
    m_Threshold      = 1e-17;
    m_MaxIterations  = 30;
    m_GenerateLeft   = SVD_M::GenerateLeft;
    m_GenerateRight  = SVD_M::GenerateRight;
}

void SVD_M_Block::SetParameters()
{
    m_SVD_M->Threshold      = m_Threshold;
    m_SVD_M->MaxIterations  = m_MaxIterations;
    m_SVD_M->m_GenerateLeft   = m_GenerateLeft;
    m_SVD_M->m_GenerateRight  = m_GenerateRight;
}

// ============================================================================
// 枚举转换
// ============================================================================

SVD_M::GenerateLeftE SVD_M_Block::ConvertStringToGenerateLeft(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "do not generate left singular vectors" || lower == "0") {
        return SVD_M::DoNotGenerateLeft;
    }
    if (lower == "generate left singular vectors" || lower == "1") {
        return SVD_M::GenerateLeft;
    }
    return SVD_M::GenerateLeft;
}

SVD_M::GenerateRightE SVD_M_Block::ConvertStringToGenerateRight(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "do not generate right singular vectors" || lower == "0") {
        return SVD_M::DoNotGenerateRight;
    }
    if (lower == "generate right singular vectors" || lower == "1") {
        return SVD_M::GenerateRight;
    }
    return SVD_M::GenerateRight;
}

// ============================================================================
// transpose
// ============================================================================

void SVD_M_Block::transpose(const DoubleMatrix& A, DoubleMatrix& AT)
{
    const size_t r = rows_of(A);
    const size_t c = cols_of(A);
    AT.Resize(static_cast<int>(c), static_cast<int>(r));
    for (size_t i = 0; i < r; ++i)
        for (size_t j = 0; j < c; ++j)
            AT(static_cast<int>(j), static_cast<int>(i)) = A(static_cast<int>(i), static_cast<int>(j));
}

// ============================================================================
// calc_svd (Golub-Reinsch SVD, m >= n)
// ============================================================================

void SVD_M_Block::calc_svd(const DoubleMatrix& A,
                           DoubleMatrix& Uo,
                           DoubleMatrix& Wo,
                           DoubleMatrix& Vo,
                           double threshold,
                           int maxIters,
                           int needV)
{
    const int m = static_cast<int>(rows_of(A));
    const int n = static_cast<int>(cols_of(A));

    Uo.Resize(m, n);
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            Uo(i, j) = A(i, j);

    std::vector<double> w(static_cast<size_t>(n), 0.0);
    std::vector<double> rv1(static_cast<size_t>(n), 0.0);

    if (needV) Vo.Resize(n, n);
    else       Vo.Resize(0, 0);

    double g = 0.0, scale = 0.0, anorm = 0.0;

    // Householder reduction
    for (int i = 0; i < n; ++i)
    {
        int l = i + 1;
        rv1[static_cast<size_t>(i)] = scale * g;
        g = 0.0;
        double s = 0.0;
        scale = 0.0;

        if (i < m)
        {
            for (int k = i; k < m; ++k) scale += std::fabs(Uo(k, i));
            if (scale > 0.0)
            {
                for (int k = i; k < m; ++k)
                {
                    Uo(k, i) /= scale;
                    s += Uo(k, i) * Uo(k, i);
                }
                double f = Uo(i, i);
                g = -SIGN(std::sqrt(s), f);
                double h = f * g - s;
                Uo(i, i) = f - g;

                for (int j = l; j < n; ++j)
                {
                    s = 0.0;
                    for (int k = i; k < m; ++k) s += Uo(k, i) * Uo(k, j);
                    double f2 = (h != 0.0) ? (s / h) : 0.0;
                    for (int k = i; k < m; ++k) Uo(k, j) += f2 * Uo(k, i);
                }
                for (int k = i; k < m; ++k) Uo(k, i) *= scale;
            }
        }

        w[static_cast<size_t>(i)] = scale * g;

        g = 0.0; s = 0.0; scale = 0.0;
        if (i < m && i != n - 1)
        {
            for (int k = l; k < n; ++k) scale += std::fabs(Uo(i, k));
            if (scale > 0.0)
            {
                for (int k = l; k < n; ++k)
                {
                    Uo(i, k) /= scale;
                    s += Uo(i, k) * Uo(i, k);
                }
                double f = Uo(i, l);
                g = -SIGN(std::sqrt(s), f);
                double h = f * g - s;
                Uo(i, l) = f - g;

                for (int k = l; k < n; ++k)
                    rv1[static_cast<size_t>(k)] = (h != 0.0) ? (Uo(i, k) / h) : 0.0;

                for (int j = l; j < m; ++j)
                {
                    s = 0.0;
                    for (int k = l; k < n; ++k) s += Uo(j, k) * Uo(i, k);
                    for (int k = l; k < n; ++k) Uo(j, k) += s * rv1[static_cast<size_t>(k)];
                }
                for (int k = l; k < n; ++k) Uo(i, k) *= scale;
            }
        }

        anorm = std::max(anorm, std::fabs(w[static_cast<size_t>(i)]) + std::fabs(rv1[static_cast<size_t>(i)]));
    }

    // Accumulate right-hand transformation
    if (needV)
    {
        for (int i = n - 1; i >= 0; --i)
        {
            int l = i + 1;
            g = rv1[static_cast<size_t>(i)];
            if (i < n - 1)
            {
                if (g != 0.0)
                {
                    for (int j = l; j < n; ++j)
                        Vo(j, i) = (Uo(i, j) / Uo(i, l)) / g;

                    for (int j = l; j < n; ++j)
                    {
                        double s2 = 0.0;
                        for (int k = l; k < n; ++k) s2 += Uo(i, k) * Vo(k, j);
                        for (int k = l; k < n; ++k) Vo(k, j) += s2 * Vo(k, i);
                    }
                }
                for (int j = l; j < n; ++j) { Vo(i, j) = 0.0; Vo(j, i) = 0.0; }
            }
            Vo(i, i) = 1.0;
        }
    }

    // Accumulate left-hand transformation
    for (int i = n - 1; i >= 0; --i)
    {
        int l = i + 1;
        g = w[static_cast<size_t>(i)];
        for (int j = l; j < n; ++j) Uo(i, j) = 0.0;

        if (g != 0.0)
        {
            double ginvt = 1.0 / g;
            for (int j = l; j < n; ++j)
            {
                double s2 = 0.0;
                for (int k = l; k < m; ++k) s2 += Uo(k, i) * Uo(k, j);
                double f = (Uo(i, i) != 0.0) ? ((s2 / Uo(i, i)) * ginvt) : 0.0;
                for (int k = i; k < m; ++k) Uo(k, j) += f * Uo(k, i);
            }
            for (int j = i; j < m; ++j) Uo(j, i) *= ginvt;
        }
        else
        {
            for (int j = i; j < m; ++j) Uo(j, i) = 0.0;
        }
        Uo(i, i) += 1.0;
    }

    const double eps = std::numeric_limits<double>::epsilon();
    const double tol = std::max(threshold * anorm, eps * anorm);

    // Diagonalize bidiagonal form
    for (int k = n - 1; k >= 0; --k)
    {
        for (int its = 0; its < maxIters; ++its)
        {
            int l = 0;
            bool flag = true;

            for (l = k; l >= 0; --l)
            {
                if (std::fabs(rv1[static_cast<size_t>(l)]) <= tol) { flag = false; break; }
                if (l == 0) break;
                if (std::fabs(w[static_cast<size_t>(l - 1)]) <= tol) break;
            }

            if (flag && l == 0) flag = false;

            if (flag)
            {
                double c2 = 0.0, s2 = 1.0;
                const int nm = l - 1;

                for (int i = l; i <= k; ++i)
                {
                    double f = s2 * rv1[static_cast<size_t>(i)];
                    rv1[static_cast<size_t>(i)] = c2 * rv1[static_cast<size_t>(i)];
                    if (std::fabs(f) <= tol) break;

                    double g2 = w[static_cast<size_t>(i)];
                    double h = hypot_(f, g2);
                    w[static_cast<size_t>(i)] = h;
                    h = (h != 0.0) ? (1.0 / h) : 0.0;
                    c2 = g2 * h;
                    s2 = -f * h;

                    for (int j = 0; j < m; ++j)
                    {
                        double y = Uo(j, nm);
                        double z = Uo(j, i);
                        Uo(j, nm) = y * c2 + z * s2;
                        Uo(j, i) = z * c2 - y * s2;
                    }
                }
            }

            double z = w[static_cast<size_t>(k)];
            if (l == k)
            {
                if (z < 0.0)
                {
                    w[static_cast<size_t>(k)] = -z;
                    if (needV)
                        for (int j = 0; j < n; ++j) Vo(j, k) = -Vo(j, k);
                }
                break;
            }

            if (its == maxIters - 1)
            {
                LOG_ERROR("SVD_M: no convergence within MaxIterations.");
            }

            int nm = k - 1;
            double x = w[static_cast<size_t>(l)];
            double y2 = w[static_cast<size_t>(nm)];
            double g2 = rv1[static_cast<size_t>(nm)];
            double h = rv1[static_cast<size_t>(k)];

            double denom = 2.0 * h * y2;
            if (std::fabs(denom) < 1e-300) denom = (denom >= 0.0) ? 1e-300 : -1e-300;

            double f = ((y2 - z) * (y2 + z) + (g2 - h) * (g2 + h)) / denom;
            double g3 = hypot_(f, 1.0);
            f = ((x - z) * (x + z) + h * (y2 / (f + SIGN(g3, f)) - h)) / (x + 1e-300);

            double c2 = 1.0, s2 = 1.0;
            for (int j = l; j <= nm; ++j)
            {
                int i = j + 1;
                double gv = rv1[static_cast<size_t>(i)];
                y2 = w[static_cast<size_t>(i)];

                double h2 = s2 * gv;
                gv = c2 * gv;

                double z2 = hypot_(f, h2);
                rv1[static_cast<size_t>(j)] = z2;
                c2 = (z2 != 0.0) ? (f / z2) : 1.0;
                s2 = (z2 != 0.0) ? (h2 / z2) : 0.0;

                f = x * c2 + gv * s2;
                gv = gv * c2 - x * s2;
                h2 = y2 * s2;
                y2 = y2 * c2;

                if (needV)
                {
                    for (int jj = 0; jj < n; ++jj)
                    {
                        double xx = Vo(jj, j);
                        double zz2 = Vo(jj, i);
                        Vo(jj, j) = xx * c2 + zz2 * s2;
                        Vo(jj, i) = zz2 * c2 - xx * s2;
                    }
                }

                z2 = hypot_(f, h2);
                w[static_cast<size_t>(j)] = z2;
                c2 = (z2 != 0.0) ? (f / z2) : 1.0;
                s2 = (z2 != 0.0) ? (h2 / z2) : 0.0;

                f = c2 * gv + s2 * y2;
                x = c2 * y2 - s2 * gv;

                for (int jj = 0; jj < m; ++jj)
                {
                    double yy3 = Uo(jj, j);
                    double zz3 = Uo(jj, i);
                    Uo(jj, j) = yy3 * c2 + zz3 * s2;
                    Uo(jj, i) = zz3 * c2 - yy3 * s2;
                }
            }

            rv1[static_cast<size_t>(l)] = 0.0;
            rv1[static_cast<size_t>(k)] = f;
            w[static_cast<size_t>(k)] = x;
        }
    }

    // 按奇异值降序排序
    for (int i = 0; i < n - 1; ++i)
    {
        int k = i;
        double p = w[static_cast<size_t>(i)];
        for (int j = i + 1; j < n; ++j)
        {
            if (w[static_cast<size_t>(j)] > p) { k = j; p = w[static_cast<size_t>(j)]; }
        }
        if (k != i)
        {
            std::swap(w[static_cast<size_t>(i)], w[static_cast<size_t>(k)]);
            swap_cols(Uo, i, k);
            if (needV) swap_cols(Vo, i, k);
        }
    }

    // 符号规范化：pivot 为正
    if (needV)
    {
        for (int c = 0; c < n; ++c)
        {
            int pr = pivot_row_maxabs(Vo, c);
            if (Vo(pr, c) < 0.0)
            {
                scale_col(Vo, c, -1.0);
                scale_col(Uo, c, -1.0);
            }
        }
    }
    else
    {
        for (int c = 0; c < n; ++c)
        {
            int pr = pivot_row_maxabs(Uo, c);
            if (Uo(pr, c) < 0.0)
                scale_col(Uo, c, -1.0);
        }
    }

    // Build diagonal W matrix (n×n)
    Wo.Resize(n, n);
    Wo.Zero();
    for (int i = 0; i < n; ++i)
        Wo(i, i) = (std::fabs(w[static_cast<size_t>(i)]) < threshold) ? 0.0 : w[static_cast<size_t>(i)];
}
