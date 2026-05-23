#ifndef IID_GAUSSIAN_BLOCK_H
#define IID_GAUSSIAN_BLOCK_H

#include "Block.h"
#include "IID_Gaussian.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API IID_Gaussian_Block : public SystemVueModelBuilder::Block
{
public:
    IID_Gaussian_Block(const std::string& name);
    ~IID_Gaussian_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    IID_Gaussian::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    IID_Gaussian::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    IID_Gaussian::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    std::unique_ptr<IID_Gaussian> m_iidGaussian;

    double m_stdDev;
    double m_offset;
    IID_Gaussian::SelectedShowAdvancedParams m_showAdvancedParams;
    IID_Gaussian::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;
    IID_Gaussian::SelectedBurstMode m_burstMode;
    int m_burstLength;
    int m_burstPeriod;
    int m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(IID_Gaussian_Block);

#endif // IID_GAUSSIAN_BLOCK_H
