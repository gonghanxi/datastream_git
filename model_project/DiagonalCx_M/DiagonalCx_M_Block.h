#ifndef DIAGONALCX_M_BLOCK_H
#define DIAGONALCX_M_BLOCK_H

#include "Block.h"
#include "DiagonalCx_M.h"

#include <complex>
#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API DiagonalCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    DiagonalCx_M_Block(const std::string& name);
    ~DiagonalCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    DiagonalCx_M::ShowAdvancedEnum ConvertStringToShowAdvancedEnum(const std::string& value);
    DiagonalCx_M::SampleRateOptionEnum ConvertStringToSampleRateOptionEnum(const std::string& value);

    SystemVueModelBuilder::DComplexMatrix m_DiagonalElements;
    DiagonalCx_M::ShowAdvancedEnum m_ShowAdvancedParams;
    DiagonalCx_M::SampleRateOptionEnum m_SampleRateOption;
    double m_SampleRate;
    int m_InitialDelay;
    int m_produced;

    std::unique_ptr<DiagonalCx_M> m_DiagonalCx_M;
};

RegAlgo(DiagonalCx_M_Block);

#endif // DIAGONALCX_M_BLOCK_H
