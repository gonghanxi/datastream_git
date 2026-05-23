#pragma once

#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Commutator : public SystemVueModelBuilder::DFModel
{
public:
    using BufferType = SystemVueModelBuilder::CircularBuffer<double>;
    using BusType = SystemVueModelBuilder::CircularBufferBusT<BufferType>;

    Commutator();

    bool Setup() override;
    bool Run() override;

    BusType input;
    BufferType output;

    int BlockSize;
    size_t m_iBlockSize;

    DECLARE_MODEL_INTERFACE(Commutator);
};
