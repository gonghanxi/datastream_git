#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SubInt : public SystemVueModelBuilder::DFModel {
public:
    DECLARE_MODEL_INTERFACE(SubInt);
    SubInt();
    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<int> pos;
    SystemVueModelBuilder::CircularBuffer<int> output;
    SystemVueModelBuilder::CircularBufferBusT<
        SystemVueModelBuilder::CircularBuffer<int>
    > neg;
};
