#ifndef RADAR_MTD_BLOCK_H
#define RADAR_MTD_BLOCK_H

#include "RADAR_MTD.h"
#include "Block.h"
#include <fftw3.h>  // FFTW库头文件
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MTD_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MTD_Block(const std::string& name);
    ~RADAR_MTD_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RADAR_MTD::SelectedWindowType ConvertStringToSelectedWindowType(const std::string& value);
    void SetDefaultParameters();

    // FFT相关函数
    bool InitializeFFTW();
    bool ExternalFFT(std::vector<std::complex<double>>& x);

    // 模型参数
    double m_PRI;                           // 脉冲重复间隔
    double m_SampleRate;                     // 采样率
    int m_NumOfPulse;                        // 脉冲数量
    RADAR_MTD::SelectedWindowType m_WindowType;  // 窗函数类型
    double* m_Freq_Weight;                    // 频率权重数组
    int m_Freq_Weight_Size;                   // 频率权重数组大小
    double* m_WindowParameters;               // 窗函数参数数组
    int m_WindowParameters_Size;              // 窗函数参数数组大小

    // 内部计算值
    int m_samplesPerPulse;                    // 每个脉冲的采样点数
    int m_totalSamples;                       // 总采样点数

    // FFTW相关变量
    fftw_plan m_fft_plan;                     // FFT计划
    fftw_complex* m_fftw_input;                // FFT输入数组
    fftw_complex* m_fftw_output;               // FFT输出数组
    bool m_fftw_initialized;                   // FFTW初始化标志

    std::unique_ptr<RADAR_MTD> m_RADAR_MTD;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<std::complex<double>>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_MTD_Block);

#endif // RADAR_MTD_BLOCK_H
