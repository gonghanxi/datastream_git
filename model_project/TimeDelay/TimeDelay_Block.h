#ifndef TIMEDELAY_BLOCK_H
#define TIMEDELAY_BLOCK_H

#include "Block.h"
#include "TimeDelay.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TimeDelay_Block : public SystemVueModelBuilder::Block
{
public:
    TimeDelay_Block(const std::string& name);
    ~TimeDelay_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    bool UpdateLatency();
    TimeDelay::UnitEnum ConvertStringToUnitEnum(const std::string& value);

    std::unique_ptr<TimeDelay> m_timeDelay;

    TimeDelay::UnitEnum m_unit;
    double m_T;
    int m_N;
    double m_delaySeconds;
    bool m_latencyReady;

    SimuParameter simulator_param;
};

RegAlgo(TimeDelay_Block);

#endif // TIMEDELAY_BLOCK_H
