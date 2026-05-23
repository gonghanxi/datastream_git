#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "iir/Iir.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API HPF_ChebyshevII : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedOrderType { Auto, UserDefined };

    DECLARE_MODEL_INTERFACE(HPF_ChebyshevII);

    HPF_ChebyshevII();

    ERESULT PropagateCharacterizationFrequency();
    bool Setup() override;
    bool Run() override;

    std::complex<double> complexExponential(double f_c, double t);
    double dBToPowerRatio(double dB);

    Iir::ChebyshevII::HighPass<20> shelfFilterReal;
    Iir::ChebyshevII::HighPass<20> shelfFilterImag;

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;

    double Loss;
    double PassFreq;
    double PassAtten;
    double StopFreq;
    double StopRipple;
    SelectedOrderType OrderType;
    int Order;

    double SampleRate;
    int FilterOrder;
    double fc;
};
