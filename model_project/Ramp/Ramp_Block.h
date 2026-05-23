#ifndef RAMP_BLOCK_H
#define RAMP_BLOCK_H

#include "Block.h"
#include "Ramp.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Ramp_Block : public SystemVueModelBuilder::Block
{
public:
    Ramp_Block(const std::string& name);
    ~Ramp_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    int GetBatchSize() const override;
    int RunBatch(int maxCount) override;

private:
    void SetDefaultParamters();
    void SetParameters();

    Ramp::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    Ramp::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    std::unique_ptr<Ramp> m_ramp;

    double m_stepPerSample;
    double m_initialValue;
    Ramp::SelectedShowAdvancedParams m_showAdvancedParams;
    Ramp::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;

    SimuParameter simulator_param;

    int m_batchSize = 10;
    size_t m_producedCount = 0;
};

RegAlgo(Ramp_Block);

#endif // RAMP_BLOCK_H
