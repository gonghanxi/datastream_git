#include "SystemVue.h"
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "../ModelDesign/iir/Iir.h"

class SYSTEMVUEMODELBUILDER_API BPF_Butterworth : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedOrderType { Auto, UserDefined };
    enum SelectedTransform { Bilinear, ImpulseInvariance };
    enum SelectedUnderSampledModel { ModelAsAllpass, ErrorOut };

    DECLARE_MODEL_INTERFACE(BPF_Butterworth);

    BPF_Butterworth();

    ERESULT PropagateCharacterizationFrequency();
    bool Setup() override;
    bool Run()   override;

    std::complex<double> complexExponential(double f_c, double t);
    double dBToPowerRatio(double dB);

    Iir::Butterworth::LowPass<20> shelfFilterReal;
    Iir::Butterworth::LowPass<20> shelfFilterImag;

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;

    double Loss;
    double FCenter;
    double PassBandwidth;
    double PassAtten;
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
