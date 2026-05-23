#pragma once
#include "BPF_ChebyshevI.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BPF_ChebyshevI_Block : public SystemVueModelBuilder::Block
{
public:
    BPF_ChebyshevI_Block(const std::string& name);
    ~BPF_ChebyshevI_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    BPF_ChebyshevI::SelectedOrderType ConvertStringToOrderType(const std::string& value);
    BPF_ChebyshevI::SelectedTransform ConvertStringToTransform(const std::string& value);
    BPF_ChebyshevI::SelectedUnderSampledModel ConvertStringToUnderSampledModel(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<BPF_ChebyshevI> m_bpf;

    double m_loss;
    double m_fCenter;
    double m_passBandwidth;
    double m_passRipple;
    double m_stopBandwidth;
    double m_stopAtten;
    BPF_ChebyshevI::SelectedOrderType m_orderType;
    int m_order;
    BPF_ChebyshevI::SelectedTransform m_transform;
    BPF_ChebyshevI::SelectedUnderSampledModel m_underSampledModel;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(BPF_ChebyshevI_Block);

