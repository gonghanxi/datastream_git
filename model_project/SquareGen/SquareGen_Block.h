#ifndef SQUAREGEN_BLOCK_H
#define SQUAREGEN_BLOCK_H

#include "Block.h"
#include "SquareGen.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SquareGen_Block : public SystemVueModelBuilder::Block
{
public:
    SquareGen_Block(const std::string& name);
    ~SquareGen_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    SquareGen::SelectedPolarity ConvertStringToPolarity(const std::string& value);
    SquareGen::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    SquareGen::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
    SquareGen::SelectedBurstMode ConvertStringToBurstMode(const std::string& value);

    std::unique_ptr<SquareGen> m_squareGen;

    double m_loLevel;
    double m_hiLevel;
    double m_frequency;
    double m_phase;
    double m_dutyCycle;
    SquareGen::SelectedPolarity m_polarity;
    SquareGen::SelectedShowAdvancedParams m_showAdvancedParams;
    SquareGen::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    double m_initialDelay;
    SquareGen::SelectedBurstMode m_burstMode;
    double m_burstLength;
    double m_burstPeriod;
    double m_burstDelay;

    SimuParameter simulator_param;
};

RegAlgo(SquareGen_Block);

#endif // SQUAREGEN_BLOCK_H
