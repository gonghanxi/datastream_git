#ifndef CONSTFXP_BLOCK_H
#define CONSTFXP_BLOCK_H

#include "Block.h"
#include "ConstFxp.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ConstFxp_Block : public SystemVueModelBuilder::Block
{
public:
    ConstFxp_Block(const std::string& name);
    ~ConstFxp_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    ConstFxp::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    ConstFxp::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    std::unique_ptr<ConstFxp> m_constFxp;

    double m_value;
    int m_fxpPos;
    ConstFxp::SelectedShowAdvancedParams m_showAdvancedParams;
    ConstFxp::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;

    SimuParameter simulator_param;
};

RegAlgo(ConstFxp_Block);

#endif // CONSTFXP_BLOCK_H
