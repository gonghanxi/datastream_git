#ifndef RADAR_KALMAN_BLOCK_H
#define RADAR_KALMAN_BLOCK_H

#include "Block.h"
#include "RADAR_Kalman.h"

#include <cmath>
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Kalman_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Kalman_Block(const std::string& name);
    ~RADAR_Kalman_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ===== 三维 IMM-Kalman 每轴状态 =====
    struct AxisIMM
    {
        bool hasFirst;
        double zPrev;
        double mu[3];           // 模型概率: 0=CV, 1=CA(q2), 2=CA(q3)
        double x1[2];           // CV 状态 [p, v]
        double P1[2][2];
        double x2[3];           // CA 状态 [p, v, a]
        double P2[3][3];
        double x3[3];
        double P3[3][3];
    };

    static const double kTiny;
    static const double kProbTiny;
    static const double kPi;

    // ===== 参数 =====
    double m_Period;
    double m_Meas_err_var;

    // 矩阵参数展平数组（column-major）
    std::vector<double> m_r_arr;   int m_r_matSize;
    std::vector<double> m_a1_arr;  int m_a1_matSize;
    std::vector<double> m_h1_arr;  int m_h1_matSize;
    std::vector<double> m_g1_arr;  int m_g1_matSize;
    std::vector<double> m_a2_arr;  int m_a2_matSize;
    std::vector<double> m_h2_arr;  int m_h2_matSize;
    std::vector<double> m_g2_arr;  int m_g2_matSize;
    std::vector<double> m_q1_arr;  int m_q1_matSize;
    std::vector<double> m_q2_arr;  int m_q2_matSize;
    std::vector<double> m_q3_arr;  int m_q3_matSize;
    std::vector<double> m_p_arr;   int m_p_matSize;
    std::vector<double> m_u_arr;   int m_u_matSize;

    std::unique_ptr<RADAR_Kalman> m_algo;

    // ===== Kalman 内部状态 =====
    unsigned long long m_sampleIndex;
    AxisIMM m_axisX;
    AxisIMM m_axisY;

    // ===== 端口缓冲区 =====

    // ===== TimeDrivenRun =====
    std::vector<double> m_inputBufferX;
    std::vector<double> m_inputBufferY;
    std::queue<double>  m_outputQueueX;
    std::queue<double>  m_outputQueueY;

    // ===== 移植自 RADAR_Kalman 的成员函数 =====
    double get_R_axis_(int axis) const;
    double get_Pmarkov_(int r, int c) const;
    void   get_u0_(double u[3]) const;

    void extract_cv_matrices_(int axis,
        double A[2][2], double H[2], double G[2],
        double& q, double& R) const;

    void extract_ca_matrices_(int axis,
        const double* qPtr, int qSize,
        double A[3][3], double H[3], double G[3],
        double& q, double& R) const;

    void reset_axis_(AxisIMM& f) const;
    void init_axis_from_first_sample_(AxisIMM& f, double z) const;
    void init_axis_from_second_sample_(AxisIMM& f, double z) const;

    void mix_axis_(const AxisIMM& f,
        double c[3],
        double mixX1[2], double mixP1[2][2],
        double mixX2[3], double mixP2[3][3],
        double mixX3[3], double mixP3[3][3]) const;

    double process_axis_(AxisIMM& f, double z, int axis);
};

RegAlgo(RADAR_Kalman_Block);

#endif // RADAR_KALMAN_BLOCK_H
