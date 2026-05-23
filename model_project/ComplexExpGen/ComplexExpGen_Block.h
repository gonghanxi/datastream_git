#ifndef COMPLEXEXPGEN_BLOCK_H
#define COMPLEXEXPGEN_BLOCK_H

#include "Block.h"
#include "ComplexExpGen.h"
#include <complex>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ComplexExpGen_Block : public SystemVueModelBuilder::Block
{
public:
    ComplexExpGen_Block(const std::string& name);
    ~ComplexExpGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    ComplexExpGen::SelectedQuadraturePolarity ConvertStringToQuadraturePolarity(const std::string& value);
    ComplexExpGen::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    ComplexExpGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    ComplexExpGen::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    std::complex<double> ParseComplex(const std::string& value);

    std::unique_ptr<ComplexExpGen> m_complexExpGen;

    double m_amplitude;
    std::complex<double> m_offset;
    double m_frequency;
    double m_phase;
    ComplexExpGen::SelectedQuadraturePolarity m_quadraturePolarity;
    ComplexExpGen::SelectedShowAdvancedParams m_showAdvancedParams;
    ComplexExpGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;
    ComplexExpGen::SelectedBurstMode m_burstMode;
    double m_burstLength;
    double m_burstPeriod;
    double m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(ComplexExpGen_Block);

#endif // COMPLEXEXPGEN_BLOCK_H
