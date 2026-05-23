#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVue.h"
#include <complex>
#include <vector>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API DelayCx : public SystemVueModelBuilder::DFModel {
public:
    using cdouble = std::complex<double>;
    enum OutputTimingEnum { EqualToInput = 0, BeforeInput = 1 };

    DECLARE_MODEL_INTERFACE(DelayCx);

    DelayCx();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<cdouble> input;
    SystemVueModelBuilder::CircularBuffer<cdouble> output;

    int N;
    OutputTimingEnum OutputTiming;

private:
    std::vector<cdouble> buf_;
    std::size_t head_;
    int warmup_;
};
