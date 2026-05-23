#ifndef FFTPRE_H
#define FFTPRE_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <LapackMat.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
struct SplitDouble{
   double mantissa, exponent;
};
class FFTPre
{

public:
    FFTPre();

    static void testFFt();
    static void testFFtNd();
    static double genSignal(std::vector<double> &signal, const std::vector<double> &base_freqs, const std::vector<double> &amps);
    int test(const std::vector<double> &frequencies);
    static long long gcd(long long a, long long b);
    static long long gcd_array(long long *numbers, int n);
    static double compute_common_period(const std::vector<double> &frequencies, int n, double precision);
    static SplitDouble splitzDouble(double value);
    static double calcFs(double maxFreq);
    static double genComplexSignal(std::vector<_lapack_complex_double> &signal, const std::vector<double> &baseFreqs, const std::vector<double> &amps);
    static void testFFtComplex();
    double genSignalComplex(std::vector<_lapack_complex_double> &signal, const std::vector<double> &baseFreqs, const std::vector<double> &amps);
};

//// 带通采样参数
//const double BANDWIDTH = 100e6; // 假设每个信号100MHz带宽
//const double CENTER_FREQ = 2.0e9; // 中心频率
//#include <complex>
//// 下变频本地振荡器
//class DownConverter {
//public:
//    DownConverter()
//    {
//        phase = 0.0;
//        phase_increment=0;
//    }
//    DownConverter(double freq) : lo_freq(freq) {
//        phase = 0.0;
//        phase_increment = 2 * M_PI * lo_freq / sampling_rate;
//    }

//    std::complex<double> mix(double sample, double t) {
//        double lo_i = cos(phase);
//        double lo_q = -sin(phase); // 负号产生下变频

//        // 更新相位
//        phase += phase_increment;
//        if (phase > 2 * M_PI) phase -= 2 * M_PI;

//        return std::complex<double>(sample * lo_i, sample * lo_q);
//    }

//    static void setSamplingRate(double fs) {
//        sampling_rate = fs;
//    }
//    static void test();

//private:
//    double lo_freq;
//    double phase;
//    double phase_increment;
//    static double sampling_rate;
//};



#endif // FFTPRE_H
