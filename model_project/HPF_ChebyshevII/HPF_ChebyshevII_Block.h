#pragma once
#include "HPF_ChebyshevII.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API HPF_ChebyshevII_Block : public SystemVueModelBuilder::Block
{
public:
    HPF_ChebyshevII_Block(const std::string& name);
    ~HPF_ChebyshevII_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    HPF_ChebyshevII::SelectedOrderType ConvertStringToOrderType(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<HPF_ChebyshevII> m_hpf;

    double m_loss;
    double m_passFreq;
    double m_passAtten;
    double m_stopFreq;
    double m_stopRipple;
    HPF_ChebyshevII::SelectedOrderType m_orderType;
    int m_order;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(HPF_ChebyshevII_Block);

