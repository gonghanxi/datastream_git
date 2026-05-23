#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include <complex>
#include <numeric>
#include <vector>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API RADAR_NLFM : public SystemVueModelBuilder::DFModel
{
public:
    enum NLF_Types { Hamming, Cos4, Gauss, Polynomial };

    DECLARE_MODEL_INTERFACE(RADAR_NLFM);

    RADAR_NLFM();

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    std::vector<double> linspace(double d1, double d2, int n);
    std::vector<double> generateWindow(int N, NLF_Types windowType);
    std::vector<double> cumsum(const std::vector<double>& input);
    std::vector<double> interp1(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& xi, const std::string& method);
    std::vector<double> cumtrapz(const std::vector<double>& x, const std::vector<double>& y);

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

    double Pulsewidth;
    double PRI;
    double Bandwidth;
    double SampleRate;
    NLF_Types NLF_Type;
    SystemVueModelBuilder::Matrix<double> Polynomial_Coef;

    int counter;
    std::vector<std::complex<double>> signal;
};
