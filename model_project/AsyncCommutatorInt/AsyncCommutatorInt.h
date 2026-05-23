#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API AsyncCommutatorInt : public SystemVueModelBuilder::DFModel
{
public:
    using BufferType = SystemVueModelBuilder::CircularBuffer<int>;
    using BusType = SystemVueModelBuilder::CircularBufferBusT<BufferType>;

    AsyncCommutatorInt();

    bool Setup() override;
    bool Run() override;

    BusType input;
    BufferType output;

    SystemVueModelBuilder::Matrix<int> BlockSizes;

    DECLARE_MODEL_INTERFACE(AsyncCommutatorInt);
};
