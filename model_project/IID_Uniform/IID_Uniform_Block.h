#ifndef IID_UNIFORM_BLOCK_H
#define IID_UNIFORM_BLOCK_H

#include "Block.h"
#include "IID_Uniform.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API IID_Uniform_Block : public SystemVueModelBuilder::Block
{
public:
    IID_Uniform_Block(const std::string& name);
    ~IID_Uniform_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    IID_Uniform::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    IID_Uniform::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    IID_Uniform::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    std::unique_ptr<IID_Uniform> m_iidUniform;

    double m_loLevel;
    double m_hiLevel;
    IID_Uniform::SelectedShowAdvancedParams m_showAdvancedParams;
    IID_Uniform::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;
    IID_Uniform::SelectedBurstMode m_burstMode;
    int m_burstLength;
    int m_burstPeriod;
    int m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(IID_Uniform_Block);

#endif // IID_UNIFORM_BLOCK_H
