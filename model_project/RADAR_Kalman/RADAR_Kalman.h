#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <cmath>
#include <algorithm>

// 与 SystemVue 2020 内置 RADAR_Kalman 黑盒行为对齐的等效实现。
// 域：Untimed Data Flow。
// 端口顺序按帮助文档：
//   输入端口 1：x_in，real
//   输入端口 2：y_in，real
//   输出端口 3：x_out，real
//   输出端口 4：y_out，real
//
// 核心实现：二维量测输入的三模型 IMM-Kalman。
// 为避免 x/y 默认情况下错误耦合，本版内部按 x 轴、y 轴各自独立执行一套
// 一维 IMM-Kalman；当修改 h1/h2 这类交叉观测矩阵时，只作为近似支持。
class SYSTEMVUEMODELBUILDER_API RADAR_Kalman : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_Kalman);
	RADAR_Kalman();

	bool Setup() override;
	bool Run() override;

	// --------- 输入端口 ---------
	SystemVueModelBuilder::CircularBuffer<double> x_in;
	SystemVueModelBuilder::CircularBuffer<double> y_in;

	// --------- 输出端口 ---------
	SystemVueModelBuilder::CircularBuffer<double> x_out;
	SystemVueModelBuilder::CircularBuffer<double> y_out;

	// --------- 参数 ---------
	double Period;
	double Meas_err_var;

	double* r_mat;
	int     r_matSize;

	double* a1_mat;
	int     a1_matSize;

	double* h1_mat;
	int     h1_matSize;

	double* g1_mat;
	int     g1_matSize;

	double* a2_mat;
	int     a2_matSize;

	double* h2_mat;
	int     h2_matSize;

	double* g2_mat;
	int     g2_matSize;

	double* q1_mat;
	int     q1_matSize;

	double* q2_mat;
	int     q2_matSize;

	double* q3_mat;
	int     q3_matSize;

	double* p_mat;
	int     p_matSize;

	double* u_mat;
	int     u_matSize;

private:
	struct AxisIMM
	{
		bool hasFirst;
		double zPrev;

		// 模型概率：0=CV，1=CA(q2)，2=CA(q3)
		double mu[3];

		// CV 一维状态：[p, v]
		double x1[2];
		double P1[2][2];

		// CA 一维状态：[p, v, a]
		double x2[3];
		double P2[3][3];
		double x3[3];
		double P3[3][3];
	};

	static const double kTiny;
	static const double kProbTiny;
	static const double kPi;

	unsigned long long sampleIndex_;
	AxisIMM axisX_;
	AxisIMM axisY_;

	// --------- 参数读取 ---------
	static double get_array_value_(const double* p, int size, int idx, double defval);

	// SystemVue 2020 的矩阵型 Float array 在 C++ 参数指针里通常按 column-major 展开。
	// 本函数返回 row-major 语义下的 (r,c)。defval 必须按 row-major 给出。
	static double get_mat_value_(const double* p, int size,
		int rows, int cols,
		int r, int c,
		const double* defval);

	double get_R_axis_(int axis) const;
	double get_Pmarkov_(int r, int c) const;
	void   get_u0_(double u[3]) const;

	void extract_cv_matrices_(int axis,
		double A[2][2],
		double H[2],
		double G[2],
		double& q,
		double& R) const;

	void extract_ca_matrices_(int axis,
		const double* qPtr,
		int qSize,
		double A[3][3],
		double H[3],
		double G[3],
		double& q,
		double& R) const;

	// --------- 初始化 ---------
	void reset_axis_(AxisIMM& f) const;
	void init_axis_from_first_sample_(AxisIMM& f, double z) const;
	void init_axis_from_second_sample_(AxisIMM& f, double z) const;

	// --------- IMM / Kalman 核心 ---------
	static void normalize_mu_(double mu[3]);
	static void sym2_(double P[2][2]);
	static void sym3_(double P[3][3]);
	static void positive_guard2_(double P[2][2]);
	static void positive_guard3_(double P[3][3]);

	void mix_axis_(const AxisIMM& f,
		double c[3],
		double mixX1[2], double mixP1[2][2],
		double mixX2[3], double mixP2[3][3],
		double mixX3[3], double mixP3[3][3]) const;

	static void predict_update_cv_(double x[2],
		double P[2][2],
		const double A[2][2],
		const double H[2],
		const double G[2],
		double q,
		double R,
		double z,
		double& likelihood);

	static void predict_update_ca_(double x[3],
		double P[3][3],
		const double A[3][3],
		const double H[3],
		const double G[3],
		double q,
		double R,
		double z,
		double& likelihood);

	double process_axis_(AxisIMM& f, double z, int axis);
};
