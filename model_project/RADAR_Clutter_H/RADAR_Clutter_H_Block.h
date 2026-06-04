#ifndef RADAR_CLUTTER_H_BLOCK_H
#define RADAR_CLUTTER_H_BLOCK_H

#include "Block.h"
#include "RADAR_Clutter_H.h"

#include <complex>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Clutter_H_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Clutter_H_Block(const std::string& name);
    ~RADAR_Clutter_H_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static int ConvertStringToPDF(const std::string& value);
    static int ConvertStringToPSD(const std::string& value);

    // 算法辅助函数
    void generateClutter(int numSample);
    void generateGaussianPSD(double fr);

    static double average(const std::vector<double>& a);
    static double variance(const std::vector<double>& a);
    static double stddev(const std::vector<double>& a);
    static std::vector<double> linearConvolve(const std::vector<double>& a, const std::vector<double>& b);

    std::unique_ptr<RADAR_Clutter_H> m_algo;

    // ===== 参数 =====
    double m_RF_Freq;
    int    m_PDF;         // SelectedPDF 枚举值
    double m_VA;
    double m_VB;
    int    m_PSD;         // SelectedPSD 枚举值
    double m_PA;
    double m_PB;
    double m_TStep;
    int    m_FilterLen;
    double m_DurationTime;
    double m_Vr;

    // ===== 缓存与状态 =====
    std::vector<double> m_filterCoeff;           // PSD 滤波器系数（实部）
    std::vector<std::complex<double>> m_clutter; // 生成的杂波序列
    int m_cachedNumSample;                       // 杂波缓存对应的 numSample
    std::mt19937 m_rng;

    // ===== TimeDrivenRun 逐点累积 =====
    std::vector<EnvelopeSignal> m_inputBuffer;
    std::queue<EnvelopeSignal>  m_outputQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;

    static constexpr double kPi = 3.14159265358979323846;
};

RegAlgo(RADAR_Clutter_H_Block);

#endif // RADAR_CLUTTER_H_BLOCK_H
