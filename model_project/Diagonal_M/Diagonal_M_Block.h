#ifndef DIAGONAL_M_BLOCK_H
#define DIAGONAL_M_BLOCK_H

#include "Block.h"
#include "Diagonal_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Diagonal_M_Block : public SystemVueModelBuilder::Block
{
public:
    Diagonal_M_Block(const std::string& name);
    ~Diagonal_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    Diagonal_M::ShowAdvancedEnum ConvertStringToShowAdvancedEnum(const std::string& value);
    Diagonal_M::SampleRateOptionEnum ConvertStringToSampleRateOptionEnum(const std::string& value);

    SystemVueModelBuilder::DoubleMatrix m_DiagonalElements;
    Diagonal_M::ShowAdvancedEnum m_ShowAdvancedParams;
    Diagonal_M::SampleRateOptionEnum m_SampleRateOption;
    double m_SampleRate;
    int m_InitialDelay;
    int m_produced;

    std::unique_ptr<Diagonal_M> m_Diagonal_M;
};

RegAlgo(Diagonal_M_Block);

#endif // DIAGONAL_M_BLOCK_H
