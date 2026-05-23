#pragma once

#include "BPF_Butterworth.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BPF_Butterworth_Block : public SystemVueModelBuilder::Block
{
public:
    BPF_Butterworth_Block(const std::string& name);
    ~BPF_Butterworth_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    BPF_Butterworth::SelectedOrderType ConvertStringToOrderType(const std::string& value);
    BPF_Butterworth::SelectedTransform ConvertStringToTransform(const std::string& value);
    BPF_Butterworth::SelectedUnderSampledModel ConvertStringToUnderSampledModel(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<BPF_Butterworth> m_bpf;

    double m_loss;
    double m_fCenter;
    double m_passBandwidth;
    double m_passAtten;
    double m_stopBandwidth;
    double m_stopAtten;
    BPF_Butterworth::SelectedOrderType m_orderType;
    int m_order;
    BPF_Butterworth::SelectedTransform m_transform;
    BPF_Butterworth::SelectedUnderSampledModel m_underSampledModel;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(BPF_Butterworth_Block);

