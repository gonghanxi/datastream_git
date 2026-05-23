#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <complex>
#include <vector>

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API SDomainIIR : public DFModel
    {
    public:
        SDomainIIR();
        virtual ~SDomainIIR();

        DECLARE_MODEL_INTERFACE(SDomainIIR);

        bool Initialize() override;
        bool Run() override;
        bool Finalize() override;

        CircularBuffer<double> input;
        CircularBuffer<double> output;

        double SampleRate;
        double Factor;

        double* RealPoles;
        int RealPolesSize;
        std::complex<double>* ComplexConjugatePoles;
        int ComplexConjugatePolesSize;

        double* RealZeros;
        int RealZerosSize;
        std::complex<double>* ComplexConjugateZeros;
        int ComplexConjugateZerosSize;

        enum FreqUnitEnum
        {
            FREQ_RAD_PER_SEC = 0,
            FREQ_HZ = 1
        };
        FreqUnitEnum FreqUnit;

        const std::vector<double>& GetA() const { return m_a; }
        const std::vector<double>& GetB() const { return m_b; }
        std::vector<double>& GetState() { return m_state; }

    private:
        std::vector<double> m_b;
        std::vector<double> m_a;
        std::vector<double> m_state;
    };
}
