#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include "TimedDFModel.h"
#include "SystemVue.h"
#include <map>
#include <complex>

class SYSTEMVUEMODELBUILDER_API RADAR_BarkerCode : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum CodeLengthEnum {
        Length_2_a,
        Length_2_b,
        Length_3,
        Length_4_a,
        Length_4_b,
        Length_5,
        Length_7,
        Length_11,
        Length_13
    };

    DECLARE_MODEL_INTERFACE(RADAR_BarkerCode);

    RADAR_BarkerCode();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

    double PRI;
    double SubPulseWidth;
    CodeLengthEnum CodeLength;
    double SampleRate;

    void InitializeBarkerCodes(std::map<CodeLengthEnum, std::vector<int>>& codes) {
        codes[Length_2_a] = { 1, 1 };
        codes[Length_2_b] = { 1, -1 };
        codes[Length_3] = { 1, 1, -1 };
        codes[Length_4_a] = { 1, 1, 1, -1 };
        codes[Length_4_b] = { 1, 1, -1, 1 };
        codes[Length_5] = { 1, 1, 1, -1, 1 };
        codes[Length_7] = { 1, 1, 1, -1, -1, 1, -1 };
        codes[Length_11] = { 1, 1, 1, -1, -1, -1, 1, -1, -1, 1, -1 };
        codes[Length_13] = { 1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1 };
    }

private:
    int counter;
    std::map<CodeLengthEnum, std::vector<int>> barkerCodes;
};
