#include "DelayEnv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(DelayEnv)
{
    SET_MODEL_DESCRIPTION("Envelope Delay");
    SET_MODEL_SYMBOL("SYM_Delay");
    SET_MODEL_CATEGORY("Signal Processing");

    {
        SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
        port.SetDescription("input signal");
    }

    {
        SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
        port.SetDescription("output signal");
    }

    {
        SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(N);
        p.SetUnit(SystemVueModelBuilder::Units::NONE);
        p.SetDefaultValue("1");
        p.SetDescription("Sample delay size");
    }

    {
        SystemVueModelBuilder::DFParam e =
            ADD_MODEL_ENUM_PARAM(OutputTiming, OutputTimingEnum);
        e.SetUnit(SystemVueModelBuilder::Units::NONE);
        e.AddEnumeration("EqualToInput", EqualToInput);
        e.AddEnumeration("BeforeInput", BeforeInput);
        e.SetDefaultValue("EqualToInput");
        e.SetDescription("Output start time");
    }
    return true;
}
#endif

DelayEnv::DelayEnv()
    : N(1), OutputTiming(EqualToInput), head_(0), warmup_(0)
{
}

bool DelayEnv::Setup()
{
    if (N < 0) {
        POST_ERROR("N must be >= 0.");
        return false;
    }

    input.SetRate(1U);
    output.SetRate(1U);

    buf_.clear();
    head_ = 0;
    warmup_ = 0;

    if (N > 0) {
        buf_.assign(static_cast<std::size_t>(N), std::complex<double>(0.0, 0.0));
        if (OutputTiming == BeforeInput) {
            warmup_ = N;
        }
    }
    return true;
}

bool DelayEnv::Run()
{
    if (N == 0) {
        output[0U] = input[0U];
        return true;
    }

    if (OutputTiming == BeforeInput && warmup_ > 0) {
        output[0U] = std::complex<double>(0.0, 0.0);
        --warmup_;
        return true;
    }

    output[0U] = buf_[head_];
    buf_[head_] = input[0U].complex();
    head_ = (head_ + 1) % buf_.size();

    return true;
}
