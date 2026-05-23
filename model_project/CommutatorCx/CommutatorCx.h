#pragma once

#include "ModelBuilder.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API CommutatorCx : public SystemVueModelBuilder::DFModel
{
public:
    using BufferType = SystemVueModelBuilder::CircularBuffer<std::complex<double>>;
    using BusType = SystemVueModelBuilder::CircularBufferBusT<BufferType>;

    CommutatorCx();

    bool Setup() override;
    bool Run() override;

    BusType input;
    BufferType output;

    int BlockSize;
    size_t m_iBlockSize;

    DECLARE_MODEL_INTERFACE(CommutatorCx);
};
