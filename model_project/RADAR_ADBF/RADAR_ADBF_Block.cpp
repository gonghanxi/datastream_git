#include "RADAR_ADBF_Block.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const double kSteeringSign          = -1.0;
static const bool   kOutputConjugateForDBF = true;
static const double kDiagonalLoadingRelative = 1.0e-10;
static const double kDiagonalLoadingAbsolute = 1.0e-12;

RADAR_ADBF_Block::RADAR_ADBF_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_ADBF_Block::SetDefaultParamters()
{
    m_NumOfXAntElement = 16;
    m_NumOfYAntElement = 1;
    m_Dx = 0.5;
    m_Dy = 0.5;
    m_NumOfSamples = 1000;
    m_Theta = 0;
    m_Phi = 0;
    m_SampleRate = 10e6;
}

void RADAR_ADBF_Block::SetParameters()
{
    if (!m_radar_adbf) {
        return;
    }
    m_radar_adbf->NumOfXAntElement = m_NumOfXAntElement;
    m_radar_adbf->NumOfYAntElement = m_NumOfYAntElement;
    m_radar_adbf->Dx = m_Dx;
    m_radar_adbf->Dy = m_Dy;
    m_radar_adbf->NumOfSamples = m_NumOfSamples;
    m_radar_adbf->Theta = m_Theta;
    m_radar_adbf->Phi = m_Phi;
    m_radar_adbf->SampleRate = m_SampleRate;
}

bool RADAR_ADBF_Block::Setup()
{
    Block::Setup();
    SetParameters();
    return true;
}

bool RADAR_ADBF_Block::Run()
{
//    if (IsVariableStepMode()) {
//        return TimeDrivenRun();
//    }
    return DataStreamRun();
}

bool RADAR_ADBF_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_radar_adbf = std::make_unique<RADAR_ADBF>();
    SetDefaultParamters();

    try { m_NumOfXAntElement = std::stod(getParameter("NumOfXAntElement").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfXAntElement', using default value."); }
    try { m_NumOfYAntElement = std::stod(getParameter("NumOfYAntElement").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfYAntElement', using default value."); }
    try { m_Dx = std::stod(getParameter("Dx").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Dx', using default value."); }
    try { m_Dy = std::stod(getParameter("Dy").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Dy', using default value."); }
    try { m_NumOfSamples = std::stoi(getParameter("NumOfSamples").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfSamples', using default value."); }
    try { m_Theta = std::stod(getParameter("Theta").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Theta', using default value."); }
    try { m_Phi = std::stod(getParameter("Phi").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phi', using default value."); }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    SetParameters();

    AddInputPort("input",  m_radar_adbf->input,  static_cast<size_t>(m_NumOfSamples), Block::DataType::DCOMPLEX_BUS);
    AddInputPort("el",     m_radar_adbf->el,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("az",     m_radar_adbf->az,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("weight", m_radar_adbf->weight, 1, Block::DataType::DCOMPLEX_BUS);

    return true;
}

bool RADAR_ADBF_Block::DataStreamRun()
{
    std::string inPortName = GetInputPortName(0);
    auto inputData = ReadInputData<std::complex<double>>(inPortName);
    if (inputData.empty()) {
        return false;
    }

    int K = m_NumOfSamples;
    if (K < 1) { K = 1; }

    const size_t totalSamples = inputData.size();
    const int nChannels = static_cast<int>(totalSamples) / K;
    if (nChannels <= 0) {
        return false;
    }

    const int nx = getNumX();
    const int ny = getNumY();
    const int expectedM = getNumElements();
    const int M = (nChannels < expectedM) ? nChannels : expectedM;

    double thetaDeg = m_Theta;
    double phiDeg   = m_Phi;

    {
        std::string elPortName = GetInputPortName(1);
        auto elData = ReadInputData<double>(elPortName);
        if (!elData.empty()) {
            thetaDeg = elData[0];
        }
    }
    {
        std::string azPortName = GetInputPortName(2);
        auto azData = ReadInputData<double>(azPortName);
        if (!azData.empty()) {
            phiDeg = azData[0];
        }
    }

    std::vector<std::complex<double>> a;
    buildSteeringVector(nx, ny, m_Dx, m_Dy, thetaDeg, phiDeg, a);

    if (static_cast<int>(a.size()) > M) { a.resize(M); }
    if (static_cast<int>(a.size()) < M) { a.resize(M, std::complex<double>(1.0, 0.0)); }

    std::vector<std::vector<std::complex<double>>> R(
        M, std::vector<std::complex<double>>(M, std::complex<double>(0.0, 0.0)));

    for (int k = 0; k < K; ++k) {
        std::vector<std::complex<double>> x(M, std::complex<double>(0.0, 0.0));
        for (int ch = 0; ch < M; ++ch) {
            x[ch] = inputData[ch * K + k];
        }
        for (int r = 0; r < M; ++r) {
            for (int c = 0; c < M; ++c) {
                R[r][c] += x[r] * std::conj(x[c]);
            }
        }
    }

    const double invK = 1.0 / static_cast<double>(K);
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < M; ++c) {
            R[r][c] *= invK;
        }
    }

    double traceReal = 0.0;
    for (int i = 0; i < M; ++i) {
        traceReal += std::real(R[i][i]);
    }
    double avgPower = traceReal / static_cast<double>(M);
    if (!std::isfinite(avgPower) || avgPower <= 0.0) {
        avgPower = 1.0;
    }
    const double loading = kDiagonalLoadingRelative * avgPower + kDiagonalLoadingAbsolute;
    for (int i = 0; i < M; ++i) {
        R[i][i] += std::complex<double>(loading, 0.0);
    }

    std::vector<std::complex<double>> u;
    std::vector<std::complex<double>> w;
    const bool ok = solveLinearSystem(R, a, u);
    if (ok && static_cast<int>(u.size()) == M) {
        std::complex<double> denom(0.0, 0.0);
        for (int i = 0; i < M; ++i) {
            denom += std::conj(a[i]) * u[i];
        }
        if (std::abs(denom) > 1.0e-24) {
            w.resize(M);
            for (int i = 0; i < M; ++i) {
                w[i] = u[i] / denom;
            }
        } else {
            fallbackConventionalWeight(a, w);
        }
    } else {
        fallbackConventionalWeight(a, w);
    }

    std::vector<std::complex<double>> outputData;
    outputData.reserve(M);
    for (int ch = 0; ch < M; ++ch) {
        if (kOutputConjugateForDBF) {
            outputData.push_back(std::conj(w[ch]));
        } else {
            outputData.push_back(w[ch]);
        }
    }

    std::string weightPortName = GetOutputPortName(0);
    WriteOutputData(weightPortName, outputData);

    return true;
}

int RADAR_ADBF_Block::getNumX() const
{
    int n = static_cast<int>(std::floor(m_NumOfXAntElement + 0.5));
    if (n < 1) { n = 1; }
    return n;
}

int RADAR_ADBF_Block::getNumY() const
{
    int n = static_cast<int>(std::floor(m_NumOfYAntElement + 0.5));
    if (n < 1) { n = 1; }
    return n;
}

int RADAR_ADBF_Block::getNumElements() const
{
    return getNumX() * getNumY();
}

void RADAR_ADBF_Block::buildSteeringVector(
    int nx, int ny, double dx, double dy,
    double thetaDeg, double phiDeg,
    std::vector<std::complex<double>>& a) const
{
    const int M = nx * ny;
    a.assign(M, std::complex<double>(1.0, 0.0));

    const double theta = deg2rad(thetaDeg);
    const double phi   = deg2rad(phiDeg);

    const double ux = std::sin(theta) * std::cos(phi);
    const double uy = std::sin(theta) * std::sin(phi);

    int idx = 0;
    for (int iy = 0; iy < ny; ++iy) {
        for (int ix = 0; ix < nx; ++ix) {
            const double phase = kSteeringSign * 2.0 * M_PI *
                (static_cast<double>(ix) * dx * ux +
                 static_cast<double>(iy) * dy * uy);
            a[idx] = std::complex<double>(std::cos(phase), std::sin(phase));
            ++idx;
        }
    }
}

bool RADAR_ADBF_Block::solveLinearSystem(
    std::vector<std::vector<std::complex<double>>> A,
    const std::vector<std::complex<double>>& b,
    std::vector<std::complex<double>>& x) const
{
    const int n = static_cast<int>(b.size());
    if (n <= 0) { return false; }
    if (static_cast<int>(A.size()) != n) { return false; }
    for (int i = 0; i < n; ++i) {
        if (static_cast<int>(A[i].size()) != n) { return false; }
    }

    std::vector<std::vector<std::complex<double>>> aug(
        n, std::vector<std::complex<double>>(n + 1, std::complex<double>(0.0, 0.0)));

    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            aug[r][c] = A[r][c];
        }
        aug[r][n] = b[r];
    }

    for (int col = 0; col < n; ++col) {
        int pivotRow = col;
        double pivotAbs = std::abs(aug[col][col]);
        for (int r = col + 1; r < n; ++r) {
            const double v = std::abs(aug[r][col]);
            if (v > pivotAbs) {
                pivotAbs = v;
                pivotRow = r;
            }
        }
        if (pivotAbs < 1.0e-30) { return false; }
        if (pivotRow != col) {
            std::swap(aug[pivotRow], aug[col]);
        }

        const std::complex<double> pivot = aug[col][col];
        for (int c = col; c <= n; ++c) {
            aug[col][c] /= pivot;
        }

        for (int r = 0; r < n; ++r) {
            if (r == col) { continue; }
            const std::complex<double> factor = aug[r][col];
            if (std::abs(factor) == 0.0) { continue; }
            for (int c = col; c <= n; ++c) {
                aug[r][c] -= factor * aug[col][c];
            }
        }
    }

    x.assign(n, std::complex<double>(0.0, 0.0));
    for (int i = 0; i < n; ++i) {
        x[i] = aug[i][n];
    }

    return true;
}

void RADAR_ADBF_Block::fallbackConventionalWeight(
    const std::vector<std::complex<double>>& a,
    std::vector<std::complex<double>>& w) const
{
    const int M = static_cast<int>(a.size());
    w.assign(M, std::complex<double>(0.0, 0.0));
    if (M <= 0) { return; }
    for (int i = 0; i < M; ++i) {
        w[i] = a[i];
    }
}

double RADAR_ADBF_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}
