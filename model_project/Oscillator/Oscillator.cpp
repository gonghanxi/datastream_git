#include "Oscillator.h"
#include <cmath>
#include <iostream>
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Oscillator)
{
    SET_MODEL_DESCRIPTION("Oscillator with Carrier Frequency");
    SET_MODEL_SYMBOL("SYM_Oscillator");
    SET_MODEL_CATEGORY("Analog/RF");
    SET_MODEL_CATEGORY("Sources");

    {
        SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Frequency);
        param.SetDescription("RF tone frequency");
        param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
        param.SetDefaultValue("1e6");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Power);
        param.SetDescription("RF tone carrier power");
        param.SetUnit(SystemVueModelBuilder::Units::POWER);
        param.SetDefaultValue("0.010");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Phase);
        param.SetDescription("RF tone carrier phase");
        param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
        param.SetDefaultValue("0.0");
        param.SetSchematicDisplay(0);
        param.SetHideCondition("RandomPhase == 1");
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(RandomPhase, SelectedYesOrNo);
        enumParam.SetDescription("Set phase of RF tone to random value between -PI and +PI: NO, YES");
        enumParam.AddEnumeration("NO", No);
        enumParam.AddEnumeration("YES", Yes);
        enumParam.SetDefaultValue("0");
        enumParam.SetSchematicDisplay(0);
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NDensity);
        param.SetDescription("Noise spectral density added");
        param.SetUnit(SystemVueModelBuilder::Units::POWER);
        param.SetDefaultValue("0");
        param.SetSchematicDisplay(0);
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RefR);
        param.SetDescription("Reference resistance");
        param.SetUnit(SystemVueModelBuilder::Units::RESISTANCE);
        param.SetDefaultValue("50");
        param.SetSchematicDisplay(0);
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ShowAdvancedParams, SelectedYesOrNo);
        enumParam.SetDescription("Show advanced parameters: NO, YES");
        enumParam.AddEnumeration("NO", No);
        enumParam.AddEnumeration("YES", Yes);
        enumParam.SetDefaultValue("0");
        enumParam.SetSchematicDisplay(0);
        enumParam.SetUseDefault(1);
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(SampleRateOption, SelectedSampleRateOption);
        enumParam.SetDescription("Sample rate option: UnTimed, Timed from SampleRate, Timed from Schematic");
        enumParam.AddEnumeration("UnTimed", UnTimed);
        enumParam.AddEnumeration("Timed from SampleRate", TimedFromSampleRate);
        enumParam.AddEnumeration("Timed from Schematic", TimedFromSchematic);
        enumParam.SetDefaultValue("2");
        enumParam.SetHideCondition("ShowAdvancedParams ~= 1");
        enumParam.SetSchematicDisplay(0);
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
        param.SetDescription("Explicit sample rate");
        param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
        param.SetDefaultValue("Sample_Rate");
        param.SetHideCondition("SampleRateOption ~= 1 || ShowAdvancedParams ~= 1");
        param.SetSchematicDisplay(0);
        param.SetUseDefault(1);
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InitialDelay);
        param.SetDescription("Output sample delay");
        param.SetUnit(SystemVueModelBuilder::Units::TIME);
        param.SetDefaultValue("0");
        param.SetHideCondition("ShowAdvancedParams ~= 1");
        param.SetSchematicDisplay(0);
        param.SetUseDefault(1);
    }
    return true;
}
#endif

Oscillator::Oscillator()
{
}

ERESULT Oscillator::PropagateCharacterizationFrequency()
{
    bool bStatus = true;

    if (Frequency) {
        output.SetCharacterizationFrequency(Frequency);
    } else {
        std::cout << "characterization frequency must be greater than 0." << std::endl;
        bStatus = false;
    }
    return bStatus;
}

bool Oscillator::Setup()
{
    bool bStatus = true;

    if (SampleRateOption == UnTimed) {
        std::cout << "Untimed sample is not supported yet. Output index may still be time related." << std::endl;
    }

    if (SampleRateOption == TimedFromSampleRate) {
        if (SampleRate > 0) {
            output.SetSampleRate(SampleRate);
        } else {
            std::cout << "SampleRate must be greater than 0." << std::endl;
            bStatus = false;
        }
    }

    if (InitialDelay < 0) {
        std::cout << "InitialDelay must not be negtive." << std::endl;
        bStatus = false;
    }

    if (RandomPhase == Oscillator::Yes) {
        const double PI = std::acos(-1);
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<double> dU(-PI, PI);
        Phase = dU(gen);
    }

    return bStatus;
}

bool Oscillator::Run()
{
    double t = output.GetTime(0, m_iFiringCount);
    SampleRate = output.GetSampleRate();
    double StdDev = std::sqrt(NDensity * SampleRate * RefR);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dN(0, StdDev);

    if (t < InitialDelay) {
        output[0] = 0.0;
    } else {
        output[0] = 10 * std::sqrt(Power) * std::exp(std::complex<double>(0, Phase)) + std::complex<double>(dN(gen), dN(gen));
    }

    return true;
}
