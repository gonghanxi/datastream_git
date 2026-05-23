#pragma once
#include "HPF_ChebyshevI.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API HPF_ChebyshevI_Block : public SystemVueModelBuilder::Block
{
public:
    HPF_ChebyshevI_Block(const std::string& name);
    ~HPF_ChebyshevI_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    HPF_ChebyshevI::SelectedOrderType ConvertStringToOrderType(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<HPF_ChebyshevI> m_hpf;

    double m_loss;
    double m_passFreq;
    double m_passRipple;
    double m_stopFreq;
    double m_stopAtten;
    HPF_ChebyshevI::SelectedOrderType m_orderType;
    int m_order;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(HPF_ChebyshevI_Block);

