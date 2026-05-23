#pragma once
#include "LPF_ChebyshevII.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API LPF_ChebyshevII_Block : public SystemVueModelBuilder::Block
{
public:
    LPF_ChebyshevII_Block(const std::string& name);
    ~LPF_ChebyshevII_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    LPF_ChebyshevII::SelectedOrderType ConvertStringToOrderType(const std::string& value);
    LPF_ChebyshevII::SelectedUnderSampledModel ConvertStringToUnderSampledModel(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<LPF_ChebyshevII> m_lpf;

    double m_loss;
    double m_passFreq;
    double m_passAtten;
    double m_stopFreq;
    double m_stopRipple;
    LPF_ChebyshevII::SelectedOrderType m_orderType;
    int m_order;
    LPF_ChebyshevII::SelectedUnderSampledModel m_underSampledModel;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(LPF_ChebyshevII_Block);

