#ifndef IDENTITY_M_BLOCK_H
#define IDENTITY_M_BLOCK_H

#include "Block.h"
#include "Identity_M.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Identity_M_Block : public SystemVueModelBuilder::Block
{
public:
    Identity_M_Block(const std::string& name);
    ~Identity_M_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Identity_M::SelectedShowAdvancedParams ConvertStringToSelectedShowAdvancedParams(const std::string& value);
    Identity_M::SelectedSampleRateOption ConvertStringToSelectedSampleRateOption(const std::string& value);

    void SetDefaultParameters();

    int m_RowsCols;
    Identity_M::SelectedShowAdvancedParams m_ShowAdvancedParams;
    Identity_M::SelectedSampleRateOption m_SampleRateOption;
    double m_SampleRate;
    int m_InitialDelay;

    std::unique_ptr<Identity_M> m_Identity_M;
};
RegAlgo(Identity_M_Block);
#endif // IDENTITY_M_BLOCK_H
