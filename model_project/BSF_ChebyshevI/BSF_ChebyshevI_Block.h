#pragma once
#include "BSF_ChebyshevI.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BSF_ChebyshevI_Block : public SystemVueModelBuilder::Block
{
public:
    BSF_ChebyshevI_Block(const std::string& name);
    ~BSF_ChebyshevI_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    BSF_ChebyshevI::SelectedOrderType ConvertStringToOrderType(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<BSF_ChebyshevI> m_bsf;

    double m_loss;
    double m_fCenter;
    double m_passBandwidth;
    double m_passRipple;
    double m_stopBandwidth;
    double m_stopAtten;
    BSF_ChebyshevI::SelectedOrderType m_orderType;
    int m_order;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(BSF_ChebyshevI_Block);

