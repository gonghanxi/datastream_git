#include "Dirichlet_Block.h"

Dirichlet_Block::Dirichlet_Block(const std::string &name)
    :Block(name)
{

}

bool Dirichlet_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Dirichlet_Block::Run()
{
    std::vector<double> inputData;
    inputData = ReadInputData<double>(GetInputPortName(0));

    const double omega = inputData[0];
    const int    Nv = (m_N < 1) ? 1 : m_N;

    std::vector<double> outputData(inputData.size());
    outputData.reserve(inputData.size());
    outputData[0] = dirichlet_sample(omega, Nv);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Dirichlet_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Dirichlet = std::make_unique<Dirichlet>();

    SetDefaultParamters();

    try { m_N = std::stod(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }

    SetParameters();

    AddInputPort("input", m_Dirichlet->input, 1, Block::DataType::TIMED_DOUBLE);
    AddOutputPort("output", m_Dirichlet->output, 1, Block::DataType::TIMED_DOUBLE);

    if(!m_Dirichlet->Setup()) return false;

    return true;
}

void Dirichlet_Block::SetParameters()
{
    if(!m_Dirichlet) return;
    m_Dirichlet->N = m_N;
}

void Dirichlet_Block::SetDefaultParamters()
{
    m_N = 10;
}

double Dirichlet_Block::dirichlet_sample(double omega_rad, int Nval)
{
    if (Nval < 1) Nval = 1;

    constexpr double TWO_PI = 6.28318530717958647692;
    const long double x_cycles = static_cast<long double>(omega_rad) / static_cast<long double>(TWO_PI);

    const long long k_nearest = llround(x_cycles);

    long double xw = x_cycles - floor(x_cycles + 0.5L);  // (-0.5, 0.5]

    const long double pi = 3.1415926535897932384626433832795L;
    const long double den = sinl(pi * xw);

    const long double eps = 1e-12L;
    if (fabsl(den) < eps) {
        const long long e = static_cast<long long>(Nval - 1) * k_nearest;
        const long double sign = (e & 1LL) ? -1.0L : 1.0L;
        return static_cast<double>(sign);
    }

    const long double num = sinl(static_cast<long double>(Nval) * pi * xw);
    const long double y = (num / den) / static_cast<long double>(Nval);
    return static_cast<double>(y);
}
