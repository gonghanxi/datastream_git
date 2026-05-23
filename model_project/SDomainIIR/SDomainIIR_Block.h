#ifndef SDOMAINIIR_BLOCK_H
#define SDOMAINIIR_BLOCK_H

#include "Block.h"
#include "SDomainIIR.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SDomainIIR_Block : public SystemVueModelBuilder::Block
{
public:
    SDomainIIR_Block(const std::string& name);
    ~SDomainIIR_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    SDomainIIR::FreqUnitEnum ConvertStringToFreqUnit(const std::string& value);

    std::unique_ptr<SystemVueModelBuilder::SDomainIIR> m_sdomainIIR;

    double m_sampleRate;
    double m_factor;

    std::vector<double> m_realPoles;
    std::vector<std::complex<double>> m_complexPoles;
    std::vector<double> m_realZeros;
    std::vector<std::complex<double>> m_complexZeros;
    SDomainIIR::FreqUnitEnum m_freqUnit;
};

RegAlgo(SDomainIIR_Block);

#endif // SDOMAINIIR_BLOCK_H
