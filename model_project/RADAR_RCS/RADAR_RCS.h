#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_RCS : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedType
    {
        ConstValue, UniformPDF, GaussianPDF, RayleighPDF,
        LogNormalPDF, ExponentialPDF, WeibullPDF, ChiSquaredPDF,
        GammaPDF, BetaPDF, FPDF, BinomialCDF, PoissonCDF
    };

    DECLARE_MODEL_INTERFACE(RADAR_RCS);

    RADAR_RCS();

    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<double> RCS;
    SystemVueModelBuilder::EnvelopeCircularBuffer Es;

    SelectedType Type;
    double VA;
    double VB;
    double TStep;
    double DurationTime;

    double t;
    bool GenFlag;
    double RCSinDuration;
    double EsinDuration;
};
