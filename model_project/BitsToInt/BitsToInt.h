#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVue.h"
#include <cstdint>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API BitsToInt : public SystemVueModelBuilder::DFModel {
public:
    enum BitOrderEnum { LSB_first = 0, MSB_first = 1 };

    DECLARE_MODEL_INTERFACE(BitsToInt);

    BitsToInt();
    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<bool> input;
    SystemVueModelBuilder::CircularBuffer<int>  output;

    int NumBits;
    BitOrderEnum BitOrder;
};
