
#ifndef RADAR_CW_H
#define RADAR_CW_H

#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <complex>


class SYSTEMVUEMODELBUILDER_API RADAR_CW : public SystemVueModelBuilder::DFModel //时序模型用TimedDFModel
{
public:
    enum Waveform_typeEnum
    {
        Sawtooth,
        Triangle,
        UserDefined
    };

public:
    // This Macro is required for all classes derived from DFModel
    DECLARE_MODEL_INTERFACE(RADAR_CW);

    // Constructor to initialize parameters
    RADAR_CW();

    //-------- Function Overloads --------
    virtual bool	Setup();
    virtual bool	Run();

    //void InitializeConfig();

    // 模型端口定义
    double freq_out;
    SystemVueModelBuilder::CircularBuffer< std::complex<double> > waveform_out;

    // Waveform_type为Sawtooth、Triangle时的模型参数定义
    Waveform_typeEnum Waveform_type;
    double Amplitude;
    double Period;
    double LowerFreq;
    double DeltaFreq;
    double SampleRate;

    // Waveform_type为UserDefined时的模型参数定义
    double FreqUpTime;
    double FreqDownTime;
    double FreqFixTime;
    double OffTime;

    // 测试变量
    double testVAR;

    int counter; // 信号计数器

private:
    unsigned int m_rate;
};



#endif // RADAR_CW_H


