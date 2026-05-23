#pragma once
#include "LPF_ChebyshevI.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API LPF_ChebyshevI_Block : public SystemVueModelBuilder::Block
{
public:
    LPF_ChebyshevI_Block(const std::string& name);
    ~LPF_ChebyshevI_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    LPF_ChebyshevI::SelectedOrderType ConvertStringToOrderType(const std::string& value);
    LPF_ChebyshevI::SelectedTransform ConvertStringToTransform(const std::string& value);
    LPF_ChebyshevI::SelectedUnderSampledModel ConvertStringToUnderSampledModel(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<LPF_ChebyshevI> m_lpf;

    double m_loss;
    double m_passFreq;
    double m_passRipple;
    double m_stopFreq;
    double m_stopAtten;
    LPF_ChebyshevI::SelectedOrderType m_orderType;
    int m_order;
    LPF_ChebyshevI::SelectedTransform m_transform;
    LPF_ChebyshevI::SelectedUnderSampledModel m_underSampledModel;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(LPF_ChebyshevI_Block);

