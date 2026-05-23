#pragma once

#include "ModelBuilder.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API AsyncCommutatorCx : public SystemVueModelBuilder::DFModel
{
public:
    using SampleType = std::complex<double>;
    using BufferType = SystemVueModelBuilder::CircularBuffer<SampleType>;
    using BusType = SystemVueModelBuilder::CircularBufferBusT<BufferType>;

    AsyncCommutatorCx();

    bool Setup() override;
    bool Run() override;

    BusType input;
    BufferType output;

    SystemVueModelBuilder::Matrix<int> BlockSizes;

    DECLARE_MODEL_INTERFACE(AsyncCommutatorCx);
};
