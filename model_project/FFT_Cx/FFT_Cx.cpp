#include "FFT_Cx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( FFT_Cx )
{
    SET_MODEL_DESCRIPTION("Complex Fast Fourier Transform");
    SET_MODEL_SYMBOL("SYM_FFT_Cx");
    SET_MODEL_CATEGORY("Signal Processing");

    {
        SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
    }

    {
        SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FFTSize);
        param.SetDescription("Output transform size");
        param.SetDefaultValue("256");
    }

    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Size);
        param.SetDescription("Number of input samples to read");
        param.SetDefaultValue("256");
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Direction, SelectedDirection);
        enumParam.SetDescription("Direction of transform: IFFT, FFT");
        enumParam.AddEnumeration("FFT", FFT);
        enumParam.AddEnumeration("IFFT", IFFT);
        enumParam.SetDefaultValue("0");
    }

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FreqSequence, SelectedFreqSequence);
        enumParam.SetDescription("Sequence for the frequency terms: 0-pos-neg, neg-0-pos");
        enumParam.AddEnumeration("0-pos-neg", O_pos_neg);
        enumParam.AddEnumeration("neg-0-pos", neg_O_pos);
        enumParam.SetDefaultValue("0");
    }
    return true;
}
#endif

FFT_Cx::FFT_Cx()
{

}

//-----------------------------------------------------------------------------------
//	Setup
//		Port rate should be set here
//-----------------------------------------------------------------------------------
bool FFT_Cx::Setup()
{
    bool bStatus = true;

    if (Size >= 1 && FFTSize >= Size)
    {
        input.SetRate(Size);
        output.SetRate(FFTSize);
    }
    else
    {
        POST_ERROR("FFTSize and Size should meet this condition: FFTSize >= Size >= 1");
        bStatus = false;
    }

     /// 目前只支持幕2的FFT，FFTSize不为2的幕次时可能会出问题///
    if ((FFTSize & (FFTSize - 1)) != 0)
    {
        POST_WARNING("Only 2^N FFTSize is supported now. For FFTSize not equels to 2^N, performance may be insufficient.");
    }

    return bStatus;
}

// 递归法FFT
void FFT_Cx::fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert)
{
    const double PI = acos(-1);

    if (n == 1) return;

    int half = n / 2;
    SystemVueModelBuilder::Matrix< std::complex<double> > even(1, half), odd(1, half);

    for (int i = 0; i < half; i++) {
        even(i) = a(i * 2);
        odd(i) = a(i * 2 + 1);
    }

    fft(even, half, invert);
    fft(odd, half, invert);

    double angle = 2 * PI / n * (invert ? -1 : 1);
    std::complex<double> w(1), wn(cos(angle), sin(angle));

    for (int i = 0; i < half; i++) {
        a(i) = even(i) + w * odd(i);
        a(i + half) = even(i) - w * odd(i);
        if (invert) {
            a(i) /= 2;
            a(i + half) /= 2;
        }
        w *= wn;
    }
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool FFT_Cx::Run()
{
    // 该模型综合了补零，FFT，IFFT，FFTShift，IFFTShift等功能
    // FFT的执行顺序：	补零 -> FFT -> FFTShift
    // IFFT的执行顺序：	补零 -> IFFTShift -> IFFT

    SystemVueModelBuilder::Matrix< std::complex<double> >	FullSequence(1, FFTSize);
    //-----------------------------------------------------------------------------------
    for (int i = 0; i < FFTSize; i++)
    {
        if (i < Size)
        {
            FullSequence(i) = input[i];
        }
        // 输入长度小于 FFT 长度时需补零
        else
        {
            FullSequence(i) = 0.0;
        }
    }

    //-----------------------------------------------------------------------------------
    if (Direction == FFT)
    {
        // 此处进行 FFT
        fft(FullSequence, FFTSize, 1);

        // 按 FFT 的点数进行加权
        FullSequence *= FFTSize;

        // 不进行 Shift
        if (FreqSequence == O_pos_neg)
        {
            for (int i = 0; i < FFTSize; i++)
            {
                output[i] = FullSequence(i);
            }
        }

        // 进行 Shift
        else if (FreqSequence == neg_O_pos)
        {
            // FFT Shift 是向右圆周位移 FFTSize/2（向下取整）位
            for (int i = 0; i < FFTSize; i++)
            {
                int n = i - FFTSize / 2;

                output[i] = FullSequence(n >= 0 ? n : n + FFTSize);
            }
        }
    }

    //-----------------------------------------------------------------------------------
    else if (Direction == IFFT)
    {
        SystemVueModelBuilder::Matrix< std::complex<double> >	ShiftSequence(1, FFTSize);

        // 不进行 Shift
        if (FreqSequence == O_pos_neg)
        {
            for (int i = 0; i < FFTSize; i++)
            {
                ShiftSequence(i) = FullSequence(i);
            }
        }

        // 进行 Shift
        else if (FreqSequence == neg_O_pos)
        {
            // IFFT Shift是向左圆周位移 FFTSize/2（向下取整）位

            for (int i = 0; i < FFTSize; i++)
            {
                int n = i + FFTSize / 2;

                ShiftSequence(i) = FullSequence(n < FFTSize ? n : n - FFTSize);
            }
        }

        // 此处进行 IFFT
        fft(ShiftSequence, FFTSize, -1);

        // 输出时需将序列颠倒
        for (int i = 0; i < FFTSize; i++)
        {
            //output[i] = ShiftSequence(i);
            output[i] = ShiftSequence(FFTSize - i - 1);
        }

    }

    return true;
}
