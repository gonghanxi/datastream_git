#pragma once
#include "BPF_ChebyshevII.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BPF_ChebyshevII_Block : public SystemVueModelBuilder::Block
{
public:
    BPF_ChebyshevII_Block(const std::string& name);
    ~BPF_ChebyshevII_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    BPF_ChebyshevII::SelectedOrderType ConvertStringToOrderType(const std::string& value);
    BPF_ChebyshevII::SelectedUnderSampledModel ConvertStringToUnderSampledModel(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<BPF_ChebyshevII> m_bpf;

    double m_loss;
    double m_fCenter;
    double m_passBandwidth;
    double m_passAtten;
    double m_stopBandwidth;
    double m_stopRipple;
    BPF_ChebyshevII::SelectedOrderType m_orderType;
    int m_order;
    BPF_ChebyshevII::SelectedUnderSampledModel m_underSampledModel;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(BPF_ChebyshevII_Block);

