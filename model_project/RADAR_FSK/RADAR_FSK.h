#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include <complex>
#include <numeric>
#include <map>
#include <vector>
#include "TimedDFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API RADAR_FSK : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum Types { FSK, FSK_PSK };
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

    DECLARE_MODEL_INTERFACE(RADAR_FSK);

    RADAR_FSK();

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

    Types Type;
    double PRI;
    SystemVueModelBuilder::Matrix<double> FHSequence;
    SystemVueModelBuilder::Matrix<double> FSKPSKSequence;
    SystemVueModelBuilder::Matrix<double> TimeIntervals;
    double FSKPSKSubTimePeriod;
    CodeLengthEnum CodeLength;
    double SampleRate;

    int counter;

    std::map<CodeLengthEnum, std::vector<int>> barkerCodes;

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

    std::vector<std::complex<double>> signal;
};
