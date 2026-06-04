#ifndef RADAR_RX_DBS_2D_BLOCK_H
#define RADAR_RX_DBS_2D_BLOCK_H

#include "Block.h"
#include "RADAR_Rx_DBS_2D.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Rx_DBS_2D_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Rx_DBS_2D_Block(const std::string& name);
    ~RADAR_Rx_DBS_2D_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static RADAR_Rx_DBS_2D::Window_TypeEnum ConvertStringToWindowType(const std::string& value);

    // 算法辅助函数
    void rebuildCache();
    static double deg2rad(double deg);
    static double i0Bessel(double x);
    static void makeWindow(RADAR_Rx_DBS_2D::Window_TypeEnum type, int L, double beta, std::vector<double>& w);

    std::unique_ptr<RADAR_Rx_DBS_2D> m_algo;

    // ===== 参数 =====
    int    m_NumOfAntx;
    int    m_NumOfAnty;
    double m_Dx;
    double m_Dy;
    double m_Theta;
    double m_Phi;
    RADAR_Rx_DBS_2D::Window_TypeEnum m_Window_Type;
    double m_WindowParameters;

    // ===== 缓存 =====
    int m_nx;
    int m_ny;
    int m_nChExpected;
    std::vector<double> m_xPos;
    std::vector<double> m_yPos;
    std::vector<double> m_wx;
    std::vector<double> m_wy;
    std::vector<double> m_taper2d;

    // ===== TimeDrivenRun 逐点累积 =====
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<std::complex<double>> m_outputQueue;

    static constexpr double kPi    = 3.14159265358979323846;
    static constexpr double kTwoPi = 6.28318530717958647692;
};

RegAlgo(RADAR_Rx_DBS_2D_Block);

#endif // RADAR_RX_DBS_2D_BLOCK_H
