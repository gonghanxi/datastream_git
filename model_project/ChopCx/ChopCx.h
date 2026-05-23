#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include "SystemVue.h"

namespace SystemVueModelBuilder
{

class SYSTEMVUEMODELBUILDER_API ChopCx : public DFModel
{
public:
    DECLARE_MODEL_INTERFACE(ChopCx);

    ChopCx();

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<double> input;
    SystemVueModelBuilder::CircularBuffer<double> output;

    int nRead;
    int nWrite;
    int Offset;
    QueryEnum UsePastInputs;

protected:
    std::size_t iReadFrom;
    std::size_t iReadNum;
    std::size_t iReadBufSize;

    std::size_t iWriteTo;
    std::size_t iWriteNum;
    std::size_t iWriteBufSize;

    std::size_t iZeroPadFrom;
    std::size_t iZeroPadNum;

    void ComputeRange();
};

}

