#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"
#include <deque>
#include <vector>
#include <random>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API PhaseShifter : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum QuantEnum { Quant_NO = 0, Quant_Bits = 1, Quant_Custom = 2 };
    enum ErrEnum { Err_None = 0, Err_Normal = 1, Err_Uniform = 2, Err_Custom = 3 };

    DECLARE_MODEL_INTERFACE(PhaseShifter);
    PhaseShifter();

    bool Setup() override;
    bool Run() override;
    ERESULT PropagateCharacterizationFrequency() override;

    SystemVueModelBuilder::EnvelopeCircularBuffer input;
    SystemVueModelBuilder::CircularBuffer<double> control;
    SystemVueModelBuilder::EnvelopeCircularBuffer output;

    double PhaseShift;
    double InsertionLoss;
    QuantEnum Quantization;
    int NumBits;
    SystemVueModelBuilder::Matrix<double> Levels;

    ErrEnum PhaseShiftError;
    double CustomError;
    double StdDev;
    double Min;
    double Max;

    double Sensitivity;
    int HilbertFilterLength;

private:
    static constexpr double kPI = 3.14159265358979323846;

    int L_{ 64 };
    std::vector<double> h_;
    std::deque<double> x_;

    void buildHilbert(int L);
    double hilbertConv() const;
    double delayedReal() const;

    double computePhaseRad(double baseDeg);

    inline double ampScale() const { return std::pow(10.0, -InsertionLoss / 20.0); }

    std::mt19937 rngN_{ 12345 };
    std::mt19937 rngU_{ 54321 };
};
