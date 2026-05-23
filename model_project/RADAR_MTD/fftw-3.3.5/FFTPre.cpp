#include "FFTPre.h"
#include <fftw3.h>
#include <QDebug>

//#define N 8 // 采样点数
FFTPre::FFTPre()
{

}

// 计算两个整数的最大公约数 (GCD)
long long FFTPre::gcd(long long a, long long b) {
    while (b != 0) {
        long long temp = b;
        b = a % b;
        a = temp;
    }
    return llabs(a);  // 返回绝对值
}

// 计算整数数组的最大公约数
long long FFTPre::gcd_array(long long *numbers, int n) {
    long long result = numbers[0];
    for (int i = 1; i < n; i++) {
        result = gcd(result, numbers[i]);
    }
    return result;
}

// 计算最小公共周期（单位：秒）
double FFTPre::compute_common_period(const std::vector<double> &frequencies, int n, double precision) {
    // 1. 将频率转换为整数表示（避免浮点误差）
    long long *int_freqs = (long long *)malloc(n * sizeof(long long));
    long long scale = (long long)(1.0 / precision);  // 精度缩放因子

    for (int i = 0; i < n; i++) {
        // 缩放频率值并四舍五入到最接近的整数
        int_freqs[i] = (long long)round(frequencies[i] * scale);
    }

    // 2. 计算缩放后频率的最大公约数 (GCD)
    long long gcd_val = gcd_array(int_freqs, n);

    // 3. 计算基本频率分辨率 f0
    double f0 = (double)gcd_val / scale;

    // 4. 最小公共周期 = 1 / f0
    double common_period = 1.0 / f0;

    free(int_freqs);
    return common_period;
}

int FFTPre::test(const std::vector<double> & frequencies) {
        // 示例：双音系统 (1.9GHz + 2.1GHz + 100MHz)

        int n = frequencies.size();
//        int n = sizeof(frequencies) / sizeof(frequencies[0]);
        // 计算最小公共周期（精度=1Hz）
        double T_total = compute_common_period(frequencies, frequencies.size(), 1.0);

        // 输出结果
        printf("Frequencies (Hz):\n");
        for (int i = 0; i < n; i++) {
            printf("  %.5f\n", frequencies[i]/1.0e9);
        }
        printf("\nBasic Frequency Resolution f0 = %.5f gHz\n", 1.0 / (1.0e9*T_total));
        printf("Common Period T_total = %.10f seconds\n", T_total);

        return 0;
    }


void FFTPre::testFFtNd() {
    // 假设我们有3个不成倍数的基频
    int n_freqs = 3;
    double base_freqs[] = {9, 11, 77.0};

    // 采样参数
    int sampling_rate = 77*9;
    int duration = 1; // 秒
//    int total_samples = sampling_rate * duration;

    // 计算每个基频对应的周期数
    std::vector<int> periods(n_freqs);
    for (int i = 0; i < n_freqs; i++) {
        periods[i] = sampling_rate / base_freqs[i];
    }

    // 计算最小公倍数，确定多维数组形状
    int md_dims = n_freqs;
    std::vector<int> md_shape(n_freqs);

    // 使用最小公倍数确定多维形状
//    int total_lcm = 1;
    for (int i = 0; i < n_freqs; i++) {
//        total_lcm = lcm(total_lcm, periods[i]);
        md_shape[i] = periods[i];
    }

    // 计算多维数组总大小
    int md_size = 1;
    for (int i = 0; i < md_dims; i++) {
        md_size *= md_shape[i];
    }

    // 生成多维信号 - 修正后的方式
    std::vector<double> signal(md_size, 0.0);
    std::vector<int> coords(md_dims, 0);

    for (int i = 0; i < md_size; i++) {
        // 将一维索引转换为多维坐标
        int remainder = i;
        for (int dim = md_dims - 1; dim >= 0; dim--) {
            coords[dim] = remainder % md_shape[dim];
            remainder /= md_shape[dim];
        }

        // 计算每个维度的相位
        double value = 0.0;
        for (int j = 0; j < md_dims; j++) {
            double phase =  2 * M_PI * coords[j] / periods[j];
//            if (coords[j]==2)
//                value += 2*sin(phase);
//            else
                value += sin(phase);
        }
        signal[i] = value;
    }

    // 准备FFTW输入和输出
    fftw_complex *in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * md_size);
    fftw_complex *out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * md_size);

    // 填充输入数据
    for (int i = 0; i < md_size; i++) {
        in[i][0] = signal[i]; // 实部
        in[i][1] = 0.0;       // 虚部
    }

    // 创建多维FFT计划
    fftw_plan plan = fftw_plan_dft(md_dims, md_shape.data(), in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // 执行变换
    fftw_execute(plan);

    // 分析结果 - 修正后的频率计算
    printf("Corrected Multidimensional FFT results:\n");
    for (int i = 0; i < md_size; i++) {
        // 将一维索引转换为多维坐标
        int remainder = i;
        for (int dim = md_dims - 1; dim >= 0; dim--) {
            coords[dim] = remainder % md_shape[dim];
            remainder /= md_shape[dim];
        }

        // 计算实际频率 - 修正后的方式
        std::vector<double> freqs(md_dims);
        for (int j = 0; j < md_dims; j++) {
            // 考虑负频率部分
            if (coords[j] > md_shape[j] / 2) {
                freqs[j] = (coords[j] - md_shape[j]) * (sampling_rate / (double)md_shape[j]);
            } else {
                freqs[j] = coords[j] * (sampling_rate / (double)md_shape[j]);
            }
        }

        double magnitude = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        magnitude /= md_size; // 归一化

        if (magnitude<0.1)
            continue;
        // 输出结果
        printf("Index %d: Coords [", i);
        for (int j = 0; j < md_dims; j++) {
            printf("%d ", coords[j]);
        }
        printf("] Frequencies [");
        for (int j = 0; j < md_dims; j++) {
            printf("%.1f ", freqs[j]);
        }
        printf("] Magnitude: %.4f\n", magnitude);
    }

    // 清理资源
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}


//#include <tuple> // 用于返回多个值

SplitDouble FFTPre::splitzDouble(double value) {
    if (value == 0.0) {
        return {0.0, 1.0}; // 特殊处理0值
    }

    // 计算以10为底的对数
    double log10Value = std::log10(std::abs(value));

    // 计算指数部分（向下取整）
    int exponent = static_cast<int>(std::floor(log10Value));

    // 计算尾数部分
    double mantissa = value / std::pow(10.0, exponent);

    // 确保尾数在 [1,10) 范围内
    if (mantissa >= 10.0) {
        mantissa /= 10.0;
        exponent += 1;
    } else if (mantissa < 1.0) {
        mantissa *= 10.0;
        exponent -= 1;
    }

    return {mantissa, std::pow(10.0, exponent)};
}

double FFTPre::calcFs(double maxFreq)
{
   SplitDouble sDouble = splitzDouble(maxFreq);
//    std::vector<double> fsSelect ={1,2,4,5,8,10,20};  //1.00, 1.25, 1.28, 1.60, 2.00, 2.50, 2.56, 3.20, 4.00, 5.00, 5.12, 6.25, 6.40, 8.00, 10.00, 10.24, 12.50, 12.80, 16.00, 20.00
    std::vector<double> fsSelect ={1.00, 1.25, 1.28, 1.60, 2.00, 2.50, 2.56, 3.20, 4.00, 5.00, 5.12, 6.25, 6.40, 8.00, 10.00, 10.24, 12.50, 12.80, 16.00, 20.00,25};
//     qDebug()<<"eeeeeeeeeeeeeeeeeee "<<sDouble.mantissa;
    for(int i=0; i<fsSelect.size(); i++)
    {
        if ((2*sDouble.mantissa+1) < fsSelect[i])
            return fsSelect[i]*sDouble.exponent;
    }
}

double FFTPre::genComplexSignal(std::vector<_lapack_complex_double> &signal,
                         const std::vector<double> &baseFreqs, const std::vector<double> &amps)
{
    double tCommon =  compute_common_period(baseFreqs, baseFreqs.size(), 1.0);
    double maxFreq = baseFreqs.back();

    double Fs = calcFs(maxFreq);// 采样频率（需满足 Nyquist 定理）
    double duration = tCommon; // 信号持续时间（秒）
    printf("FFT duration %.4f :\n", duration*1e9);

//    Fs = 3*1e9;
    size_t N = static_cast<size_t>(Fs * duration); // 采样点数

    _lapack_complex_double v = {0,0};
    signal.resize(N);
    for (size_t i = 0; i < N; ++i) {
        double t = i / Fs;
        int f =0;
         v.real = 0;
        for (double freq : baseFreqs) {
            v.real += amps[f]*std::sin(2.0 * M_PI * freq * t);
//            signal[i] += amps[f]*std::sin(2.0 * M_PI * freq * t);
            f++;
        }
        signal[i] = v;
    }
    return Fs;
}


double FFTPre::genSignal(std::vector<double> &signal,
                         const std::vector<double> &baseFreqs, const std::vector<double> &amps)
{
    double tCommon =  compute_common_period(baseFreqs, baseFreqs.size(), 1.0);
    double maxFreq = baseFreqs.back();

    double Fs = calcFs(maxFreq);// 采样频率（需满足 Nyquist 定理）

    double duration = tCommon; // 信号持续时间（秒）
    size_t N = static_cast<size_t>(Fs * duration); // 采样点数

    signal.resize(N);
    for (size_t i = 0; i < N; ++i) {
        double t = i / Fs;
        int f =0;
        for (double freq : baseFreqs) {
            signal[i] += amps[f]*std::sin(2.0 * M_PI * freq * t);
            f++;
        }
    }
    return Fs;
}


double FFTPre::genSignalComplex(std::vector<_lapack_complex_double> &signal,
                         const std::vector<double> &baseFreqs, const std::vector<double> &amps)
{
    double tCommon =  compute_common_period(baseFreqs, baseFreqs.size(), 1.0);
    double maxFreq = baseFreqs.back();

    double Fs = calcFs(maxFreq);// 采样频率（需满足 Nyquist 定理）

    double duration = tCommon; // 信号持续时间（秒）
    size_t N = static_cast<size_t>(Fs * duration); // 采样点数

    signal.resize(N);
    for (size_t i = 0; i < N; ++i) {
        double t = i / Fs;
        int f =0;
        for (double freq : baseFreqs) {
            signal[i].real += amps[f]*std::sin(2.0 * M_PI * freq * t);
            signal[i].imag = 0;
            f++;
        }
    }
    return Fs;
}

void FFTPre::testFFtComplex()
{
    std::vector<_lapack_complex_double> signal;
    std::vector<_lapack_complex_double> spectrum;
    std::vector<double> baseFreqs = {0.32e9, 1.3e9, 1.2e9}; // 基频
    std::vector<double> amps = {1.91, 1, 2}; // 基频的振幅
    double sampling_rate =   genComplexSignal(signal, baseFreqs, amps);

    int N = signal.size();
    spectrum.resize(N);

     fftw_plan plan_c2c = fftw_plan_dft_1d(N,
                                           reinterpret_cast<fftw_complex*>(signal.data()),
                                           reinterpret_cast<fftw_complex*>(spectrum.data()), FFTW_FORWARD, FFTW_ESTIMATE);
     fftw_execute(plan_c2c);

     printf("FFT 结果（复数，共 %d 点）:\n", N);
     printf("FFT 采样率 %.4f :\n", sampling_rate/1e9);
     double fsGhz = sampling_rate/1e9;
     for (int i = 0; i < N; i++) {

         lapack_complex_double v = spectrum[i];
         double ma = sqrt(v.real*v.real+v.imag*v.imag);
//         double ma=  sqrt(spectrum[i][0]*spectrum[i][0]+spectrum[i][1]*spectrum[i][1]);
         if (ma>0.1)
         {
             if (i<=N/2)
                 printf("索引 %d: 频点 %f: %.4f \n", i, i*fsGhz/N, ma/N);
             if (i>N/2)
                 printf("索引 %d: 频点 %f: %.4f \n", i, (i-N)*fsGhz/N, ma/N);

         }
     }

     std::vector<_lapack_complex_double> times;
     times.resize(N);
     fftw_plan plan_backward = fftw_plan_dft_1d(N,
                                                reinterpret_cast<fftw_complex*>(spectrum.data()),
                                                reinterpret_cast<fftw_complex*>(times.data()),
                                                FFTW_BACKWARD, FFTW_ESTIMATE);
     fftw_execute(plan_backward);

     // 4. 归一化
     for (int i = 0; i < N; i++)
     {
//         qDebug()<<times[i].real/N<<signal[i].real;
//         printf("n %d: %.2f \n", i, output[i]/N);
     }


     fftw_destroy_plan(plan_c2c);
//     fftw_destroy_plan(inverse);
//    fftw_free(in);
//    fftw_free(out);
}
void FFTPre::testFFt()
{
    std::vector<double> signal;
    std::vector<double> baseFreqs = {0.32e9, 1.9e9, 1.2e9, 6e9}; // 基频
    std::vector<double> amps = {3, 1, 2, 5}; // 基频的振幅
    double sampling_rate =   genSignal(signal, baseFreqs, amps);

    int N = signal.size();
//    double maxFreq = baseFreqs.back();
//    double sampling_rate = 5.0e9;

    double *in = signal.data(); // 实数输入（长度 N）
//    fftw_complex *out = fftw_alloc_complex(N/2 + 1); // 复数输出（长度 N/2 + 1）
//    fftw_complex *spectrum = fftw_alloc_complex(N); // 复数输出（长度 N/2 + 1）
       std::vector<_lapack_complex_double> spectrum;
       spectrum.resize(N);
    // 2. 创建 FFT 执行计划
    fftw_plan plan = fftw_plan_dft_r2c_1d(
        N,       // 变换长度
        in,      // 输入数组
        reinterpret_cast<fftw_complex*>(spectrum.data()),     // 输出数组
        FFTW_ESTIMATE // 计划标志（快速估算最优算法）
    );


//    // 3. 初始化输入数据（示例：矩形波）
//    for (int i = 0; i < N; i++) {
//        in[i] = (i >= 2 && i < 6) ? 1.0 : 0.0; // [2,5]区间为1，其余为0
//    }

    // 4. 执行 FFT 变换
    fftw_execute(plan);

    // 5. 打印结果（实部 + 虚部）
    printf("FFT 结果（复数，共 %d 点）:\n", N/2 + 1);
    for (int i = 0; i < N; i++) {

        lapack_complex_double v = spectrum[i];
        double ma = sqrt(v.real*v.real+v.imag*v.imag);
//        double ma=  sqrt(spectrum[i][0]*spectrum[i][0]+spectrum[i][1]*spectrum[i][1]);
        if (ma>0.1)
        printf("频点 %f: %.2f \n", i*sampling_rate/N, ma/N);
    }


    double* output = fftw_alloc_real(N);
     fftw_plan inverse = fftw_plan_dft_c2r_1d(N, reinterpret_cast<fftw_complex*>(spectrum.data()), output, FFTW_ESTIMATE);
     fftw_execute(inverse);

     // 4. 归一化
     for (int i = 0; i < N; i++)
     {
         qDebug()<<output[i]/N<<signal[i];
//         printf("n %d: %.2f \n", i, output[i]/N);
     }

    // 6. 清理资源
     fftw_destroy_plan(plan);
     fftw_destroy_plan(inverse);
//    fftw_free(in);
    fftw_free(output);
//    fftw_free(spectrum);
}



