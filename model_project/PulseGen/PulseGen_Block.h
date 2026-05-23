#ifndef PULSEGEN_BLOCK_H
#define PULSEGEN_BLOCK_H

#include "PulseGen.h"
#include "Block.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PulseGen_Block : public SystemVueModelBuilder::Block
{
public:
    PulseGen_Block(const std::string& name);
    ~PulseGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    PulseGen::EdgeSymmetrys ConvertStringToEdgeSymmetrys(const std::string& value);
    PulseGen::Polaritys ConvertStringToPolaritys(const std::string& value);

    void SetDefaultParameters();

    double m_LoLevel;
    double m_HiLevel;
    double m_Period;
    double m_Phase;
    double m_PulseWidth;
    PulseGen::EdgeSymmetrys m_EdgeSymmetry;
    double m_EdgeTime;
    double m_RisingEdgeTime;
    double m_FallingEdgeTime;
    PulseGen::Polaritys m_Polarity;
    double m_SampleRate;

    std::unique_ptr<PulseGen> m_PulseGen;
};
RegAlgo(PulseGen_Block);
#endif // PULSEGEN_BLOCK_H
