#ifndef CONST_BLOCK_H
#define CONST_BLOCK_H

#include "Block.h"
#include "Const.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Const_Block : public SystemVueModelBuilder::Block
{
public:
    Const_Block(const std::string& name);
    ~Const_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    Const::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    Const::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    std::unique_ptr<Const> m_const;

    double m_value;
    Const::SelectedShowAdvancedParams m_showAdvancedParams;
    Const::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;

    SimuParameter simulator_param;
};

RegAlgo(Const_Block);

#endif // CONST_BLOCK_H
