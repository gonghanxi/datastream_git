#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "iir/Iir.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API BSF_ChebyshevI : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedOrderType { Auto, UserDefined };

    DECLARE_MODEL_INTERFACE(BSF_ChebyshevI);

    BSF_ChebyshevI();

    ERESULT PropagateCharacterizationFrequency();
    bool Setup() override;
    bool Run() override;

    std::complex<double> complexExponential(double f_c, double t);
    double dBToPowerRatio(double dB);

    Iir::ChebyshevI::HighPass<20> shelfFilterReal;
    Iir::ChebyshevI::HighPass<20> shelfFilterImag;

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;

    double Loss;
    double FCenter;
    double PassBandwidth;
    double PassRipple;
    double StopBandwidth;
    double StopAtten;
    SelectedOrderType OrderType;
    int Order;

    double SampleRate;
    int FilterOrder;
    double fc;
};
