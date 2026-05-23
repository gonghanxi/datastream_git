#ifndef IDENTITYCX_M_BLOCK_H
#define IDENTITYCX_M_BLOCK_H

#include "Block.h"
#include "IdentityCx_M.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API IdentityCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    IdentityCx_M_Block(const std::string& name);
    ~IdentityCx_M_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    IdentityCx_M::SelectedShowAdvancedParams ConvertStringToSelectedShowAdvancedParams(const std::string& value);
    IdentityCx_M::SelectedSampleRateOption ConvertStringToSelectedSampleRateOption(const std::string& value);

    void SetDefaultParameters();

    int m_RowsCols;
    IdentityCx_M::SelectedShowAdvancedParams m_ShowAdvancedParams;
    IdentityCx_M::SelectedSampleRateOption m_SampleRateOption;
    double m_SampleRate;
    int m_InitialDelay;

    std::unique_ptr<IdentityCx_M> m_IdentityCx_M;
};
RegAlgo(IdentityCx_M_Block);

#endif // IDENTITYCX_M_BLOCK_H
