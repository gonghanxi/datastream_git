#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API MpyInt : public SystemVueModelBuilder::DFModel {
public:
    DECLARE_MODEL_INTERFACE(MpyInt);
    MpyInt();
    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBufferBusT<
        SystemVueModelBuilder::CircularBuffer<int>
    > input;
    SystemVueModelBuilder::CircularBuffer<int> output;
};
