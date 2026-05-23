#pragma once
#include "HPF_Butterworth.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API HPF_Butterworth_Block : public SystemVueModelBuilder::Block
{
public:
    HPF_Butterworth_Block(const std::string& name);
    ~HPF_Butterworth_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    HPF_Butterworth::SelectedOrderType ConvertStringToOrderType(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<HPF_Butterworth> m_hpf;

    double m_loss;
    double m_passFreq;
    double m_passAtten;
    double m_stopFreq;
    double m_stopAtten;
    HPF_Butterworth::SelectedOrderType m_orderType;
    int m_order;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(HPF_Butterworth_Block);

