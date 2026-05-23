
#include "RADAR_CW.h"
//#include "CDFInterfaceImplementation.h"
#include <cmath>
#include <QDebug>
#include <iostream>


#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_CW)
{
    SET_MODEL_DESCRIPTION("Radar CW waveform generation");
    SET_MODEL_CATEGORY("Signal Source");

    unsigned int rate = 3;
    SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(waveform_out);
    port.SetPosition("1,0.33");
    port.SetRateValue(rate);
    m_rate = port.GetImplementation()->GetRateValue();


    ADD_MODEL_OUTPUT(freq_out);

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Waveform_type,Waveform_typeEnum);

        enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
        enumParam.AddEnumeration("Sawtooth", Sawtooth);
        enumParam.AddEnumeration("Triangle", Triangle);
        enumParam.AddEnumeration("UserDefined", UserDefined);
        enumParam.SetDefaultValue("Sawtooth");
        enumParam.SetDescription("Radar CW waveform Type, Sawtooth and Triangle is the type of frequency change. UserDefined mode is used to define more complex FMCW signal.: Sawtooth, Triangle, UserDefined");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Amplitude);
        param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
        param.SetDefaultValue("1");
        param.SetDescription("Generated waveform magnitude");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Period);
        param.SetUnit(SystemVueModelBuilder::Units::TIME);
        param.SetDefaultValue("1e-4");
        param.SetDescription("Waveform period");
        param.SetHideCondition("Waveform_type ~= 0 && Waveform_type ~= 1");
    }
    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FreqUpTime);
        param.SetUnit(SystemVueModelBuilder::Units::TIME);
        param.SetDefaultValue("1e-5");
        param.SetDescription("The period of waveform frequency upward.");
        param.SetHideCondition("Waveform_type ~= 2");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FreqDownTime);
        param.SetUnit(SystemVueModelBuilder::Units::TIME);
        param.SetDefaultValue("1e-5");
        param.SetDescription("The period of waveform frequency downward.");
        param.SetHideCondition("Waveform_type ~= 2");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FreqFixTime);
        param.SetUnit(SystemVueModelBuilder::Units::TIME);
        param.SetDefaultValue("1e-5");
        param.SetDescription("The period of waveform frequency keeps no change.");
        param.SetHideCondition("Waveform_type ~= 2");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(OffTime);
        param.SetUnit(SystemVueModelBuilder::Units::TIME);
        param.SetDefaultValue("1e-5");
        param.SetDescription("The period of waveform output is off.");
        param.SetHideCondition("Waveform_type ~= 2");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(LowerFreq);
        param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
        param.SetDefaultValue("10e3");
        param.SetDescription("Start Frequency");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(DeltaFreq);
        param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
        param.SetDefaultValue("50e3");
        param.SetDescription("Bandwidth");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
        param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
        param.SetDefaultValue("1e6");
        param.SetDescription("Baseband waveform sample rate");

    }

    // __SaveConfig_RADAR_CW(model);
    // if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) {
    //     std::string fileName = "E:/project/archermind/RADAR_CW_complete_config.json";
    //     model.GetImplementation()->SaveCompleteConfig(fileName);
    // }

    return true;
}
#endif

RADAR_CW::RADAR_CW()
{
    counter = 0;

    //InitializeConfig();

}
// void RADAR_CW::InitializeConfig()
// {
//     try {
//         // 使用模型的类名作为配置键
//         SystemVueModelBuilder::DFInterface interface("RADAR_CW");

//         // 调用接口定义
//         DefineInterface(interface);

//         qDebug() << "RADAR_CW configuration initialized successfully";
//     } catch (const std::exception& e) {
//         qDebug() << "Configuration initialization failed:" << e.what();
//     }
// }
bool RADAR_CW::Setup()
{
    bool bStatus = true;
    if (SampleRate > 0)
    {
        // Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
        //output.SetSampleRate(SampleRate);
    }
    else
    {
        POST_ERROR("SampleRate must be greater than 0.");
        bStatus = false;
    }
    return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_CW::Run()
{

    bool bStatus = true;

    const double PI = 3.14159265358979323846;
    double time = counter / SampleRate;
    double frequency;
    std::complex<double> waveform;

    switch (Waveform_type){
    case Sawtooth:
    {
        double t = fmod(time, Period - 2.0e-19);
        frequency = LowerFreq + (t / Period) * DeltaFreq;
        double Phase = 2 * PI * (LowerFreq * time + 0.5 * DeltaFreq * (t * t / Period));
        waveform = Amplitude * std::complex<double>(std::cos(Phase), std::sin(Phase));

        break;
    }
    case Triangle:
    {
        double t = fmod(time, Period - 1.0e-19);
        double halfPeriod = Period / 2.0;
        frequency = LowerFreq + (t <= halfPeriod ? (t / halfPeriod) : (1.0 - (t - halfPeriod) / halfPeriod)) * DeltaFreq;
        double Phase = 2 * PI * (LowerFreq * time + 0.5 * DeltaFreq * ((t <= halfPeriod) ? (t * t / halfPeriod) : ((t - halfPeriod) * (t - halfPeriod) / halfPeriod)));
        waveform = Amplitude * std::complex<double>(std::cos(Phase), std::sin(Phase));
        break;
    }
    case UserDefined:
    {
        double totalPeriod = FreqUpTime + FreqDownTime + FreqFixTime + OffTime;
        double t = fmod(time, totalPeriod - 1.0e-19);
        double phase = 0.0;

        if (t < FreqUpTime) {
            // FreqUpTime: Linearly increase frequency
            frequency = LowerFreq + (t / FreqUpTime) * DeltaFreq;
            phase = 2 * PI * (LowerFreq * time + 0.5 * DeltaFreq * (t * t / FreqUpTime));
        }
        else if (t < FreqUpTime + FreqFixTime) {
            // FreqDownTime: Linearly decrease frequency
            frequency = LowerFreq + DeltaFreq;
            double upPhase = 2 * PI * (LowerFreq * FreqUpTime + 0.5 * DeltaFreq * FreqUpTime);
            phase = upPhase + 2 * PI * DeltaFreq * (time - FreqUpTime);
        }
        else if (t < FreqUpTime + FreqDownTime + FreqFixTime) {
            // FreqFixTime: Hold frequency constantFreqFixTime - FreqUpTime) / FreqDownTime) * DeltaFreq;
            frequency = LowerFreq + DeltaFreq - ((t - FreqFixTime - FreqUpTime) / FreqDownTime) * DeltaFreq;
            double fixPhase = 2 * PI * (LowerFreq * (FreqUpTime + FreqFixTime) + DeltaFreq * FreqFixTime);
            phase = fixPhase + 2 * PI * (DeltaFreq * (time - FreqUpTime - FreqFixTime) - 0.5 * DeltaFreq * ((t - FreqFixTime - FreqUpTime) * (t - FreqFixTime - FreqUpTime) / FreqDownTime));
        }

        else {
            // OffTime: Frequency is 0
            frequency = 0.0;
            waveform = std::complex<double>(0.0, 0.0);
            break;
        }

        waveform = Amplitude * std::complex<double>(std::cos(phase), std::sin(phase));

        break;
    }
    }
    freq_out = frequency;
    waveform_out[0].real(waveform.real());
    waveform_out[0].imag(waveform.imag());
    counter++;
    //testVAR = code[codeIndex]; // 测试输出
    return bStatus;
}


