#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include "Matrix.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API RADAR_PulseCompression : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedWindowType { Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser };

    DECLARE_MODEL_INTERFACE(RADAR_PulseCompression);

    RADAR_PulseCompression();

    bool Setup() override;
    bool Run() override;

    void fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert);
    int factorial(int n);
    double I0(int n, double x);

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> signal;
    SystemVueModelBuilder::CircularBuffer<std::complex<double>> reference;
    SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

    int Samplenum;
    int FFTSize;
    double Bandwidth;
    double SampleRate;
    SelectedWindowType WindowType;
    double WindowParameter;
};
