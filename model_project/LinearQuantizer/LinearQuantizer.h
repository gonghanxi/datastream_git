#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <cstddef>
#include <cmath>
#include <limits>
#include <string>

class SYSTEMVUEMODELBUILDER_API LinearQuantizer : public SystemVueModelBuilder::DFModel {
public:
    DECLARE_MODEL_INTERFACE(LinearQuantizer);

    LinearQuantizer();
    bool Setup() override;
    bool Run()   override;

    SystemVueModelBuilder::CircularBuffer<double> input;
    SystemVueModelBuilder::CircularBuffer<int> step;
    SystemVueModelBuilder::CircularBuffer<double> amp;

    int    Levels;
    double Low;
    double High;
};
