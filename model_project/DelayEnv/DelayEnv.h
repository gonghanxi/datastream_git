#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"
#include <complex>
#include <vector>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API DelayEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum OutputTimingEnum { EqualToInput = 0, BeforeInput = 1 };

    DECLARE_MODEL_INTERFACE(DelayEnv);

    DelayEnv();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::EnvelopeCircularBuffer input;
    SystemVueModelBuilder::EnvelopeCircularBuffer output;

    int N;
    OutputTimingEnum OutputTiming;

private:
    std::vector<std::complex<double>> buf_;
    std::size_t head_;
    int warmup_;
};
