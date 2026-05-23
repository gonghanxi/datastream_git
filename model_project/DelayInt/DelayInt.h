#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVue.h"
#include <vector>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API DelayInt : public SystemVueModelBuilder::DFModel {
public:
    enum OutputTimingEnum { EqualToInput = 0, BeforeInput = 1 };

    DECLARE_MODEL_INTERFACE(DelayInt);

    DelayInt();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<int> input;
    SystemVueModelBuilder::CircularBuffer<int> output;

    int N;
    OutputTimingEnum OutputTiming;

private:
    std::vector<int> buf_;
    std::size_t head_;
    int warmup_;
};
