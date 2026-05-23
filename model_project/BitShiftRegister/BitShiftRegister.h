#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"
#include <vector>

class SYSTEMVUEMODELBUILDER_API BitShiftRegister : public SystemVueModelBuilder::DFModel
{
public:
    enum BitOrderEnum
    {
        LSB_FIRST = 0,
        MSB_FIRST = 1
    };

    DECLARE_MODEL_INTERFACE(BitShiftRegister);

    BitShiftRegister();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<bool> input;
    SystemVueModelBuilder::TimedCircularBuffer<int>  clock;
    SystemVueModelBuilder::TimedCircularBuffer<int>  reset;
    SystemVueModelBuilder::TimedCircularBuffer<bool> output;

    int NumBits;
    BitOrderEnum BitOrder;

private:
    std::vector<int> reg_;

    template<typename T>
    static inline int toBit(const T& v) { return v ? 1 : 0; }
};
