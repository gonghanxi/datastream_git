#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "iir/Iir.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API BPF_ChebyshevI : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedOrderType { Auto, UserDefined };
    enum SelectedTransform { Bilinear, ImpulseInvariance };
    enum SelectedUnderSampledModel { ModelAsAllpass, ErrorOut };

    DECLARE_MODEL_INTERFACE(BPF_ChebyshevI);

    BPF_ChebyshevI();

    ERESULT PropagateCharacterizationFrequency();
    bool Setup() override;
    bool Run() override;

    std::complex<double> complexExponential(double f_c, double t);
    double dBToPowerRatio(double dB);

    Iir::ChebyshevI::LowPass<20> shelfFilterReal;
    Iir::ChebyshevI::LowPass<20> shelfFilterImag;

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;

    double Loss;
    double FCenter;
    double PassBandwidth;
    double PassRipple;
    double StopBandwidth;
    double StopAtten;
    SelectedOrderType OrderType;
    int Order;
    SelectedTransform Transform;
    SelectedUnderSampledModel UnderSampledModel;

    double SampleRate;
    int FilterOrder;
    double fc;
};
