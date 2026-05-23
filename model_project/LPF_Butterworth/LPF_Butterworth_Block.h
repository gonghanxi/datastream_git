#pragma once
#include "LPF_Butterworth.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API LPF_Butterworth_Block : public SystemVueModelBuilder::Block
{
public:
    LPF_Butterworth_Block(const std::string& name);
    ~LPF_Butterworth_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    LPF_Butterworth::SelectedOrderType ConvertStringToOrderType(const std::string& value);
    LPF_Butterworth::SelectedTransform ConvertStringToTransform(const std::string& value);
    LPF_Butterworth::SelectedUnderSampledModel ConvertStringToUnderSampledModel(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<LPF_Butterworth> m_lpf;

    double m_loss;
    double m_passFreq;
    double m_passAtten;
    double m_stopFreq;
    double m_stopAtten;
    LPF_Butterworth::SelectedOrderType m_orderType;
    int m_order;
    LPF_Butterworth::SelectedTransform m_transform;
    LPF_Butterworth::SelectedUnderSampledModel m_underSampledModel;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(LPF_Butterworth_Block);

