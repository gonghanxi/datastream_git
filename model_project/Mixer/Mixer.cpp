#include "Mixer.h"

#include <cmath>
#include <iostream>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Mixer)
{
    SET_MODEL_DESCRIPTION("Envelope Signal Mixer");
    SET_MODEL_SYMBOL("SYM_Mixer");
    SET_MODEL_CATEGORY("Analog/RF");

    ADD_MODEL_INPUT(inPort);
    ADD_MODEL_INPUT(loPort);
    ADD_MODEL_OUTPUT(outPort);

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ConvGain);
        param.SetName("ConvGain");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("0");
        param.SetDescription("Conversion gain in dB");
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(EnableNoise, EnableNoiseEnum);
        enumParam.SetName("EnableNoise");
        enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
        enumParam.AddEnumeration("NO", NO);
        enumParam.AddEnumeration("YES", YES);
        enumParam.SetDefaultValue("YES");
        enumParam.SetDescription("Enable mixer noise: NO, YES");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(NoiseFigure);
        param.SetName("NoiseFigure");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("0");
        param.SetDescription("Double sideband noise figure in dB");
        param.SetHideCondition("EnableNoise ~= 1");
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Sideband, SidebandEnum);
        enumParam.SetName("Sideband");
        enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
        enumParam.AddEnumeration("Lower", Lower);
        enumParam.AddEnumeration("Upper", Upper);
        enumParam.SetDefaultValue("Lower");
        enumParam.SetDescription("Mixer primary output sideband");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SidebandSuppression);
        param.SetName("SidebandSuppression");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("-200");
        param.SetDescription("Suppression of the output alternate sideband in dB");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RfRej);
        param.SetName("RfRej");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("-200");
        param.SetDescription("RF to output rejection in dB");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(LoRej);
        param.SetName("LoRej");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("-200");
        param.SetDescription("LO to output rejection in dB");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(LoRfIso);
        param.SetName("LoRfIso");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("-200");
        param.SetDescription("LO to RF isolation in dB");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RfLoIso);
        param.SetName("RfLoIso");
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("-200");
        param.SetDescription("RF to LO isolation in dB");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SOIout);
        param.SetName("SOIout");
        param.SetUnit(SystemVueModelBuilder::Units::POWER);
        param.SetDefaultValue("1.0e17");
        param.SetDescription("Output second order intercept power");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOIout);
        param.SetName("TOIout");
        param.SetUnit(SystemVueModelBuilder::Units::POWER);
        param.SetDefaultValue("1.0e17");
        param.SetDescription("Output third order intercept power");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RefR);
        param.SetName("RefR");
        param.SetUnit(SystemVueModelBuilder::Units::RESISTANCE);
        param.SetDefaultValue("50");
        param.SetDescription("Reference resistance");
    }
    return true;
}
#endif

Mixer::Mixer()
{
}

bool Mixer::Setup()
{
    outPort.SetRate(1);
    return true;
}

ERESULT Mixer::PropagateCharacterizationFrequency()
{
    bool bStatus = true;

    fc_inPort = inPort.GetCharacterizationFrequency();
    fc_loPort = loPort.GetCharacterizationFrequency();

    if (fc_inPort > 0 && fc_loPort > 0)
    {
        if (Sideband == Upper)
            outPort.SetCharacterizationFrequency(fc_inPort + fc_loPort);
        if (Sideband == Lower)
            outPort.SetCharacterizationFrequency(std::abs(fc_inPort - fc_loPort));
    }
    else
    {
        std::cout << "characterization frequency must be greater than 0." << std::endl;
        bStatus = false;
    }
    return bStatus;
}

bool Mixer::Run()
{
    double magInPort = std::sqrt(loPort[0].real() * loPort[0].real() + loPort[0].imag() * loPort[0].imag());
    outPort[0] = inPort[0].complex() * loPort[0].complex() / magInPort;
    return true;
}
