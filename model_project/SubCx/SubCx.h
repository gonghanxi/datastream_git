#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SubCx : public SystemVueModelBuilder::DFModel {
public:
    DECLARE_MODEL_INTERFACE(SubCx);

    SubCx();
    bool Setup() override;
    bool Run() override;

    using cdouble = std::complex<double>;

    SystemVueModelBuilder::CircularBuffer<cdouble> pos;
    SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::CircularBuffer<cdouble>> neg;
    SystemVueModelBuilder::CircularBuffer<cdouble> output;
};
