#include "SDomainIIR.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace SystemVueModelBuilder;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(SDomainIIR)
{
    ADD_MODEL_HEADER_FILE("SDomainIIR.h");
    SET_MODEL_NAMESPACE("SystemVueModelBuilder");

    SET_MODEL_DESCRIPTION("S-Domain IIR Filter Using Bilinear Transform and Cascade Biauad Structure");
    SET_MODEL_SYMBOL("SYM_SDomainSystem");
    SET_MODEL_CATEGORY("Filters");

    {
        DFPort p = ADD_MODEL_INPUT(input);
        p.SetName("input");
        p.SetDescription("input (real)");
    }
    {
        DFPort p = ADD_MODEL_OUTPUT(output);
        p.SetName("output");
        p.SetDescription("output (real)");
    }

    {
        DFParam param = ADD_MODEL_PARAM(SampleRate);
        param.SetName("SampleRate");
        param.SetDescription("Sampling rate");
        param.SetDefaultValue("1e6");
    }

    {
        DFParam param = ADD_MODEL_PARAM(Factor);
        param.SetName("Factor");
        param.SetDescription("H(s)=Factor*((s-z1)*(s-z2)*...)/((s-p1)*(s-p2)*...)");
        param.SetDefaultValue("1.1616128054708951e+029");
    }

    {
        DFParam param = ADD_MODEL_ARRAY_PARAM(RealPoles, RealPolesSize);
        param.SetName("RealPoles");
        param.SetDescription("Real Poles");
        param.SetDefaultValue("[-650148.07080641726]");
    }

    {
        DFParam param =
            ADD_MODEL_ARRAY_PARAM(ComplexConjugatePoles, ComplexConjugatePolesSize);
        param.SetName("ComplexConjugatePoles");
        param.SetDescription("Complex conjugate poles (specify only one for a complex conjugate pair)");
        param.SetDefaultValue("[-200906.80273926951+618327.55929716607j, "
            "-525980.83814247814+382147.44782641466j]");
    }

    {
        DFParam param = ADD_MODEL_ARRAY_PARAM(RealZeros, RealZerosSize);
        param.SetName("RealZeros");
        param.SetDescription("Real zeros");
        param.SetDefaultValue("[]");
    }

    {
        DFParam param =
            ADD_MODEL_ARRAY_PARAM(ComplexConjugateZeros, ComplexConjugateZerosSize);
        param.SetName("ComplexConjugateZeros");
        param.SetDescription("Complex conjugate zeros (specify only one for a complex conjugate pair)");
        param.SetDefaultValue("[]");
    }

    {
        DFParam e = ADD_MODEL_ENUM_PARAM(FreqUnit, FreqUnitEnum);
        e.SetName("FreqUnit");
        e.SetDescription("Frequency unit");
        e.AddEnumeration("Radians per second", SDomainIIR::FREQ_RAD_PER_SEC);
        e.AddEnumeration("Hz", SDomainIIR::FREQ_HZ);
        e.SetDefaultValue("Radians per second");
    }

    return true;
}
#endif

SDomainIIR::SDomainIIR()
    : SampleRate(1.0e6)
    , Factor(1.1616128054708951e+029)
    , RealPoles(nullptr)
    , RealPolesSize(0)
    , ComplexConjugatePoles(nullptr)
    , ComplexConjugatePolesSize(0)
    , RealZeros(nullptr)
    , RealZerosSize(0)
    , ComplexConjugateZeros(nullptr)
    , ComplexConjugateZerosSize(0)
    , FreqUnit(FREQ_RAD_PER_SEC)
{
}

SDomainIIR::~SDomainIIR()
{
}

bool SDomainIIR::Initialize()
{
    m_b.clear();
    m_a.clear();
    m_state.clear();

    if (SampleRate <= 0.0)
    {
        std::cout << "SDomainIIR: SampleRate must be > 0." << std::endl;
        return false;
    }

    const double freqScale = (FreqUnit == FREQ_HZ) ? (2.0 * M_PI) : 1.0;

    std::vector< std::complex<double> > zeros_s;
    std::vector< std::complex<double> > poles_s;

    for (int i = 0; i < RealZerosSize; ++i)
    {
        zeros_s.push_back(std::complex<double>(RealZeros[i] * freqScale, 0.0));
    }

    for (int i = 0; i < ComplexConjugateZerosSize; ++i)
    {
        std::complex<double> z = ComplexConjugateZeros[i] * freqScale;
        zeros_s.push_back(z);
        zeros_s.push_back(std::conj(z));
    }

    for (int i = 0; i < RealPolesSize; ++i)
    {
        poles_s.push_back(std::complex<double>(RealPoles[i] * freqScale, 0.0));
    }
    for (int i = 0; i < ComplexConjugatePolesSize; ++i)
    {
        std::complex<double> p = ComplexConjugatePoles[i] * freqScale;
        poles_s.push_back(p);
        poles_s.push_back(std::conj(p));
    }

    if (poles_s.empty())
    {
        std::cout << "SDomainIIR: At least one pole must be specified." << std::endl;
        return false;
    }

    std::complex<double> H0(Factor, 0.0);

    for (size_t i = 0; i < zeros_s.size(); ++i)
        H0 *= -zeros_s[i];

    std::complex<double> denom(1.0, 0.0);
    for (size_t i = 0; i < poles_s.size(); ++i)
        denom *= -poles_s[i];

    if (std::abs(denom) < 1e-30)
    {
        std::cout << "SDomainIIR: invalid pole configuration (denominator is zero at s=0)." << std::endl;
        return false;
    }

    H0 /= denom;

    const double k = 2.0 * SampleRate;

    std::vector< std::complex<double> > zeros_z;
    std::vector< std::complex<double> > poles_z;

    zeros_z.reserve(zeros_s.size() + poles_s.size());
    poles_z.reserve(poles_s.size());

    for (size_t i = 0; i < zeros_s.size(); ++i)
    {
        std::complex<double> s = zeros_s[i];
        std::complex<double> z = (s + k) / (k - s);
        zeros_z.push_back(z);
    }

    for (size_t i = 0; i < poles_s.size(); ++i)
    {
        std::complex<double> s = poles_s[i];
        std::complex<double> z = (s + k) / (k - s);
        poles_z.push_back(z);
    }

    if (poles_z.size() > zeros_z.size())
    {
        size_t extra = poles_z.size() - zeros_z.size();
        for (size_t i = 0; i < extra; ++i)
        {
            zeros_z.push_back(std::complex<double>(-1.0, 0.0));
        }
    }

    std::complex<double> num_z(1.0, 0.0);
    std::complex<double> den_z(1.0, 0.0);

    for (size_t i = 0; i < zeros_z.size(); ++i) {
        num_z *= (1.0 - zeros_z[i]);
    }

    for (size_t i = 0; i < poles_z.size(); ++i) {
        den_z *= (1.0 - poles_z[i]);
    }

    if (std::abs(num_z) < 1e-30)
    {
        std::cout << "SDomainIIR: digital zeros produce zero gain at z=1." << std::endl;
        return false;
    }

    std::complex<double> Gc = H0 * den_z / num_z;
    double G = Gc.real();
    if (std::abs(G) < 1e-30)
    {
        std::cout << "SDomainIIR: digital gain is (almost) zero." << std::endl;
    }
    if (std::abs(Gc.imag()) > 1e-6 * std::max(1.0, std::abs(G)))
    {
        std::cout << "SDomainIIR: complex gain encountered, using real part only." << std::endl;
    }

    const size_t Nz = zeros_z.size();
    const size_t Np = poles_z.size();

    std::vector< std::complex<double> > polyN(1, std::complex<double>(1.0, 0.0));
    for (size_t i = 0; i < Nz; ++i)
    {
        std::vector< std::complex<double> > tmp(polyN.size() + 1,
            std::complex<double>(0.0, 0.0));
        for (size_t j = 0; j < polyN.size(); ++j)
        {
            tmp[j] += polyN[j];
            tmp[j + 1] += -zeros_z[i] * polyN[j];
        }
        polyN.swap(tmp);
    }

    std::vector< std::complex<double> > polyD(1, std::complex<double>(1.0, 0.0));
    for (size_t i = 0; i < Np; ++i)
    {
        std::vector< std::complex<double> > tmp(polyD.size() + 1,
            std::complex<double>(0.0, 0.0));
        for (size_t j = 0; j < polyD.size(); ++j)
        {
            tmp[j] += polyD[j];
            tmp[j + 1] += -poles_z[i] * polyD[j];
        }
        polyD.swap(tmp);
    }

    for (size_t i = 0; i < polyN.size(); ++i)
        polyN[i] *= G;

    const size_t Nb = polyN.size();
    const size_t Na = polyD.size();

    if (Na == 0)
    {
        std::cout << "SDomainIIR: internal denominator has zero length." << std::endl;
        return false;
    }

    m_b.resize(Nb);
    m_a.resize(Na);

    for (size_t i = 0; i < Nb; ++i)
        m_b[i] = polyN[i].real();

    for (size_t i = 0; i < Na; ++i)
        m_a[i] = polyD[i].real();

    double a0 = m_a[0];
    if (std::fabs(a0) < 1e-12)
    {
        std::cout << "SDomainIIR: denominator leading coefficient is zero after mapping." << std::endl;
        return false;
    }

    for (size_t i = 0; i < Nb; ++i)
        m_b[i] /= a0;
    for (size_t i = 0; i < Na; ++i)
        m_a[i] /= a0;

    m_a[0] = 1.0;

    size_t Nstate = (Na > Nb ? Na : Nb);
    if (Nstate > 0)
        Nstate -= 1;

    m_state.assign(Nstate, 0.0);

    return true;
}

bool SDomainIIR::Run()
{
    if (m_a.empty() || m_b.empty())
    {
        output[0] = input[0];
        return true;
    }

    const int Na = static_cast<int>(m_a.size());
    const int Nb = static_cast<int>(m_b.size());
    const int N = static_cast<int>(m_state.size());

    const double x = input[0];
    double y = 0.0;

    if (N == 0)
    {
        double b0 = (Nb > 0) ? m_b[0] : 0.0;
        y = b0 * x;
    }
    else
    {
        double* s = m_state.data();

        double acc = (Nb > 0 ? m_b[0] * x : 0.0) + s[0];
        y = acc;

        for (int i = 0; i < N - 1; ++i)
        {
            double next = s[i + 1];

            if (i + 1 < Nb)
                next += m_b[i + 1] * x;

            if (i + 1 < Na)
                next -= m_a[i + 1] * y;

            s[i] = next;
        }

        double last = 0.0;
        if (N < Nb)
            last += m_b[N] * x;
        if (N < Na)
            last -= m_a[N] * y;

        s[N - 1] = last;
    }

    output[0] = y;
    return true;
}

bool SDomainIIR::Finalize()
{
    m_b.clear();
    m_a.clear();
    m_state.clear();
    return true;
}
