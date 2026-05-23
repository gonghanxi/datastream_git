#pragma once
#include "BSF_ChebyshevII.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BSF_ChebyshevII_Block : public SystemVueModelBuilder::Block
{
public:
    BSF_ChebyshevII_Block(const std::string& name);
    ~BSF_ChebyshevII_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    BSF_ChebyshevII::SelectedOrderType ConvertStringToOrderType(const std::string& value);

    void SetDefaultParamters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<BSF_ChebyshevII> m_bsf;

    double m_loss;
    double m_fCenter;
    double m_passBandwidth;
    double m_passAtten;
    double m_stopBandwidth;
    double m_stopRipple;
    BSF_ChebyshevII::SelectedOrderType m_orderType;
    int m_order;
    double m_sampleRate;SimuParameter simulator_param;
};

RegAlgo(BSF_ChebyshevII_Block);

