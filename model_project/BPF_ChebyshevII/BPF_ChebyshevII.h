#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "iir/Iir.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API BPF_ChebyshevII : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedOrderType { Auto, UserDefined };
    enum SelectedUnderSampledModel { ModelAsAllpass, ErrorOut };

    DECLARE_MODEL_INTERFACE(BPF_ChebyshevII);

    BPF_ChebyshevII();

    ERESULT PropagateCharacterizationFrequency();
    bool Setup() override;
    bool Run() override;

    std::complex<double> complexExponential(double f_c, double t);
    double dBToPowerRatio(double dB);

    Iir::ChebyshevII::LowPass<20> shelfFilterReal;
    Iir::ChebyshevII::LowPass<20> shelfFilterImag;

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;

    double Loss;
    double FCenter;
    double PassBandwidth;
    double PassAtten;
    double StopBandwidth;
    double StopRipple;
    SelectedOrderType OrderType;
    int Order;
    SelectedUnderSampledModel UnderSampledModel;

    double SampleRate;
    int FilterOrder;
    double fc;
};
