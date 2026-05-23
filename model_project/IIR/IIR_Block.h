#pragma once

#include "IIR.h"
#include "Block.h"

#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API IIR_Block : public SystemVueModelBuilder::Block
{
public:
    IIR_Block(const std::string& name);
    ~IIR_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParamters();
    void ResetState();

    std::unique_ptr<IIR> m_iir;

    double m_gain;
    std::vector<double> m_numerator;
    std::vector<double> m_denominator;

    int m_numState = 0;
    std::vector<double> m_state;
};

RegAlgo(IIR_Block);
