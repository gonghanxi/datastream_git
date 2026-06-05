#include "RADAR_Kalman.h"

#include <cmath>
#include <algorithm>
#include <iostream>

const double RADAR_Kalman::kTiny = 1.0e-12;
const double RADAR_Kalman::kProbTiny = 1.0e-300;
const double RADAR_Kalman::kPi = 3.1415926535897932384626433832795;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Kalman)
{
	SET_MODEL_DESCRIPTION("Radar Kalman Filter");
	SET_MODEL_CATEGORY("Radar");

	// --------- 输入端口 ---------
	{
		auto p = ADD_MODEL_INPUT(x_in);
		p.SetDescription("x measure value");
	}
	{
		auto p = ADD_MODEL_INPUT(y_in);
		p.SetDescription("y measure value");
	}

	// --------- 输出端口 ---------
	{
		auto p = ADD_MODEL_OUTPUT(x_out);
		p.SetDescription("x estimated value");
	}
	{
		auto p = ADD_MODEL_OUTPUT(y_out);
		p.SetDescription("y estimated value");
	}

	// --------- 参数 ---------
	{
		auto p = ADD_MODEL_PARAM(Period);
		p.SetDefaultValue("2");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Radar Scan Period");
	}
	{
		auto p = ADD_MODEL_PARAM(Meas_err_var);
		p.SetDefaultValue("10000");
		p.SetDescription("Measure Error Variance");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(r_mat, r_matSize);
		p.SetDefaultValue("[10000 0;0 10000]");
		p.SetDescription("Measure Error Variance Matrix");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(a1_mat, a1_matSize);
		p.SetDefaultValue("[1 2 0 0;0 1 0 0;0 0 1 2;0 0 0 1]");
		p.SetDescription("Status Transition Matrix for constant velocity model");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(h1_mat, h1_matSize);
		p.SetDefaultValue("[1 0 0 0;0 0 1 0]");
		p.SetDescription("Observation Matrix for constant velocity model");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(g1_mat, g1_matSize);
		p.SetDefaultValue("[1 0;1 0;0 1;0 1]");
		p.SetDescription("System Control Matrix for constant velocity model");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(a2_mat, a2_matSize);
		p.SetDefaultValue("[1 1 0 0 2 0;0 1 0 0 2 0;0 0 1 2 0 2;0 0 0 1 0 2;0 0 0 0 1 0;0 0 0 0 0 1]");
		p.SetDescription("Status Transition Matrix for constant accelerate model");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(h2_mat, h2_matSize);
		p.SetDefaultValue("[1 0 0 0 0 0;0 0 1 0 0 0]");
		p.SetDescription("Observation Matrix for constant accelerate model");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(g2_mat, g2_matSize);
		p.SetDefaultValue("[1 0;2 0;0 1;0 2;1 0;0 1]");
		p.SetDescription("System Control Matrix for constant accelerate model");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(q1_mat, q1_matSize);
		p.SetDefaultValue("[0 0;0 0]");
		p.SetDescription("Variance of accelerate for constant velocity model in the IMM algorithm");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(q2_mat, q2_matSize);
		p.SetDefaultValue("[0.05 0;0 0.05]");
		p.SetDescription("Variance of accelerate for constant accelerate model in the IMM algorithm");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(q3_mat, q3_matSize);
		p.SetDefaultValue("[0.02 0;0 0.02]");
		p.SetDescription("Another Variance of accelerate for constant accelerate model in the IMM algorithm");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(p_mat, p_matSize);
		p.SetDefaultValue("[0.95 0.025 0.025;0.025 0.95 0.025;0.025 0.025 0.95]");
		p.SetDescription("Markov Probability Transition Matrix which is used to control model transition in the IMM algorithm");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(u_mat, u_matSize);
		p.SetDefaultValue("[1;0;0]");
		p.SetDescription("To generate the initial Matrix with Markov Probability Transition Matrix in the IMM algorithm");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_Kalman::RADAR_Kalman(): Period(2.0),
	 Meas_err_var(10000.0),
	 r_mat(nullptr), r_matSize(0),
	 a1_mat(nullptr), a1_matSize(0),
	 h1_mat(nullptr), h1_matSize(0),
	 g1_mat(nullptr), g1_matSize(0),
	 a2_mat(nullptr), a2_matSize(0),
	 h2_mat(nullptr), h2_matSize(0),
	 g2_mat(nullptr), g2_matSize(0),
	 q1_mat(nullptr), q1_matSize(0),
	 q2_mat(nullptr), q2_matSize(0),
	 q3_mat(nullptr), q3_matSize(0),
	 p_mat(nullptr), p_matSize(0),
	 u_mat(nullptr), u_matSize(0),
	 sampleIndex_(0ULL)
{
	reset_axis_(axisX_);
	reset_axis_(axisY_);
}

// ---------------- 参数读取 ----------------
double RADAR_Kalman::get_array_value_(const double* p, int size, int idx, double defval)
{
	if (p == nullptr || idx < 0 || idx >= size)
		return defval;
	return p[idx];
}

double RADAR_Kalman::get_mat_value_(const double* p, int size,
	int rows, int cols,
	int r, int c,
	const double* defval)
{
	const int dstRowMajor = r * cols + c;
	const int srcColumnMajor = c * rows + r;
	return get_array_value_(p, size, srcColumnMajor, defval[dstRowMajor]);
}

double RADAR_Kalman::get_R_axis_(int axis) const
{
	static const double dR[] = { 10000.0, 0.0, 0.0, 10000.0 };
	double v = get_mat_value_(r_mat, r_matSize, 2, 2, axis, axis, dR);
	if (!std::isfinite(v) || v <= 0.0)
		v = (Meas_err_var > 0.0 && std::isfinite(Meas_err_var)) ? Meas_err_var : 10000.0;
	return v;
}

double RADAR_Kalman::get_Pmarkov_(int r, int c) const
{
	static const double dP[] = {
		0.95, 0.025, 0.025,
		0.025, 0.95, 0.025,
		0.025, 0.025, 0.95
	};
	double v = get_mat_value_(p_mat, p_matSize, 3, 3, r, c, dP);
	if (!std::isfinite(v) || v < 0.0)
		v = 0.0;
	return v;
}

void RADAR_Kalman::get_u0_(double u[3]) const
{
	u[0] = get_array_value_(u_mat, u_matSize, 0, 1.0);
	u[1] = get_array_value_(u_mat, u_matSize, 1, 0.0);
	u[2] = get_array_value_(u_mat, u_matSize, 2, 0.0);
	normalize_mu_(u);
}

void RADAR_Kalman::extract_cv_matrices_(int axis,
	double A[2][2],
	double H[2],
	double G[2],
	double& q,
	double& R) const
{
	static const double dA1[] = {
		1,2,0,0,
		0,1,0,0,
		0,0,1,2,
		0,0,0,1
	};
	static const double dH1[] = {
		1,0,0,0,
		0,0,1,0
	};
	static const double dG1[] = {
		1,0,
		1,0,
		0,1,
		0,1
	};
	static const double dQ1[] = { 0,0,0,0 };

	const int s0 = (axis == 0) ? 0 : 2;
	const int s1 = (axis == 0) ? 1 : 3;
	const int idx[2] = { s0, s1 };

	for (int r = 0; r < 2; ++r)
		for (int c = 0; c < 2; ++c)
			A[r][c] = get_mat_value_(a1_mat, a1_matSize, 4, 4, idx[r], idx[c], dA1);

	// 默认情况下 x 轴取 h1 第 0 行，y 轴取 h1 第 1 行。
	for (int c = 0; c < 2; ++c)
		H[c] = get_mat_value_(h1_mat, h1_matSize, 2, 4, axis, idx[c], dH1);

	for (int r = 0; r < 2; ++r)
		G[r] = get_mat_value_(g1_mat, g1_matSize, 4, 2, idx[r], axis, dG1);

	q = get_mat_value_(q1_mat, q1_matSize, 2, 2, axis, axis, dQ1);
	if (!std::isfinite(q) || q < 0.0)
		q = 0.0;

	R = get_R_axis_(axis);
}

void RADAR_Kalman::extract_ca_matrices_(int axis,
	const double* qPtr,
	int qSize,
	double A[3][3],
	double H[3],
	double G[3],
	double& q,
	double& R) const
{
	static const double dA2[] = {
		1,1,0,0,2,0,
		0,1,0,0,2,0,
		0,0,1,2,0,2,
		0,0,0,1,0,2,
		0,0,0,0,1,0,
		0,0,0,0,0,1
	};
	static const double dH2[] = {
		1,0,0,0,0,0,
		0,0,1,0,0,0
	};
	static const double dG2[] = {
		1,0,
		2,0,
		0,1,
		0,2,
		1,0,
		0,1
	};
	static const double dQ2[] = { 0.05,0,0,0.05 };
	static const double dQ3[] = { 0.02,0,0,0.02 };

	const int idx[3] = {
		(axis == 0) ? 0 : 2,
		(axis == 0) ? 1 : 3,
		(axis == 0) ? 4 : 5
	};

	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			A[r][c] = get_mat_value_(a2_mat, a2_matSize, 6, 6, idx[r], idx[c], dA2);

	for (int c = 0; c < 3; ++c)
		H[c] = get_mat_value_(h2_mat, h2_matSize, 2, 6, axis, idx[c], dH2);

	for (int r = 0; r < 3; ++r)
		G[r] = get_mat_value_(g2_mat, g2_matSize, 6, 2, idx[r], axis, dG2);

	const double* dq = (qPtr == q2_mat) ? dQ2 : dQ3;
	q = get_mat_value_(qPtr, qSize, 2, 2, axis, axis, dq);
	if (!std::isfinite(q) || q < 0.0)
		q = 0.0;

	R = get_R_axis_(axis);
}

// ---------------- 初始化 ----------------
void RADAR_Kalman::reset_axis_(AxisIMM& f) const
{
	f.hasFirst = false;
	f.zPrev = 0.0;

	f.mu[0] = 1.0;
	f.mu[1] = 0.0;
	f.mu[2] = 0.0;

	for (int i = 0; i < 2; ++i)
	{
		f.x1[i] = 0.0;
		for (int j = 0; j < 2; ++j)
			f.P1[i][j] = 0.0;
	}

	for (int i = 0; i < 3; ++i)
	{
		f.x2[i] = 0.0;
		f.x3[i] = 0.0;
		for (int j = 0; j < 3; ++j)
		{
			f.P2[i][j] = 0.0;
			f.P3[i][j] = 0.0;
		}
	}
}

void RADAR_Kalman::init_axis_from_first_sample_(AxisIMM& f, double z) const
{
	double u[3];
	get_u0_(u);
	f.mu[0] = u[0];
	f.mu[1] = u[1];
	f.mu[2] = u[2];

	const double Rscale = (Meas_err_var > 0.0 && std::isfinite(Meas_err_var)) ? Meas_err_var : 10000.0;
	const double Ppos = 4.0 * Rscale;
	const double Pvel = 1.0 * Rscale;
	const double Pacc = 0.25 * Rscale;

	f.x1[0] = z;
	f.x1[1] = 0.0;
	f.P1[0][0] = Ppos;
	f.P1[0][1] = 0.0;
	f.P1[1][0] = 0.0;
	f.P1[1][1] = Pvel;

	f.x2[0] = z;
	f.x2[1] = 0.0;
	f.x2[2] = 0.0;
	f.x3[0] = z;
	f.x3[1] = 0.0;
	f.x3[2] = 0.0;

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
		{
			f.P2[i][j] = 0.0;
			f.P3[i][j] = 0.0;
		}

	f.P2[0][0] = f.P3[0][0] = Ppos;
	f.P2[1][1] = f.P3[1][1] = Pvel;
	f.P2[2][2] = f.P3[2][2] = Pacc;

	f.zPrev = z;
	f.hasFirst = true;
}

void RADAR_Kalman::init_axis_from_second_sample_(AxisIMM& f, double z) const
{
	const double T = (Period > kTiny && std::isfinite(Period)) ? Period : 2.0;
	const double v = (z - f.zPrev) / T;

	f.x1[0] = z;
	f.x1[1] = v;

	f.x2[0] = z;
	f.x2[1] = v;
	f.x2[2] = 0.0;

	f.x3[0] = z;
	f.x3[1] = v;
	f.x3[2] = 0.0;

	f.zPrev = z;
}

// ---------------- 数值工具 ----------------
void RADAR_Kalman::normalize_mu_(double mu[3])
{
	for (int i = 0; i < 3; ++i)
	{
		if (!std::isfinite(mu[i]) || mu[i] < 0.0)
			mu[i] = 0.0;
	}

	double s = mu[0] + mu[1] + mu[2];
	if (s <= kTiny)
	{
		mu[0] = 1.0;
		mu[1] = 0.0;
		mu[2] = 0.0;
		return;
	}

	mu[0] /= s;
	mu[1] /= s;
	mu[2] /= s;
}

void RADAR_Kalman::sym2_(double P[2][2])
{
	const double a = 0.5 * (P[0][1] + P[1][0]);
	P[0][1] = a;
	P[1][0] = a;
}

void RADAR_Kalman::sym3_(double P[3][3])
{
	for (int i = 0; i < 3; ++i)
		for (int j = i + 1; j < 3; ++j)
		{
			const double a = 0.5 * (P[i][j] + P[j][i]);
			P[i][j] = a;
			P[j][i] = a;
		}
}

void RADAR_Kalman::positive_guard2_(double P[2][2])
{
	sym2_(P);
	for (int i = 0; i < 2; ++i)
	{
		if (!std::isfinite(P[i][i]) || P[i][i] < kTiny)
			P[i][i] = kTiny;
	}
}

void RADAR_Kalman::positive_guard3_(double P[3][3])
{
	sym3_(P);
	for (int i = 0; i < 3; ++i)
	{
		if (!std::isfinite(P[i][i]) || P[i][i] < kTiny)
			P[i][i] = kTiny;
	}
}

// ---------------- IMM 混合 ----------------
void RADAR_Kalman::mix_axis_(const AxisIMM& f,
	double c[3],
	double mixX1[2], double mixP1[2][2],
	double mixX2[3], double mixP2[3][3],
	double mixX3[3], double mixP3[3][3]) const
{
	const double* xs[3] = { f.x1, f.x2, f.x3 };
	const int dims[3] = { 2, 3, 3 };

	double Pmark[3][3];
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			Pmark[i][j] = get_Pmarkov_(i, j);

	double mixProb[3][3];
	for (int j = 0; j < 3; ++j)
	{
		c[j] = 0.0;
		for (int i = 0; i < 3; ++i)
			c[j] += Pmark[i][j] * f.mu[i];
		if (!std::isfinite(c[j]) || c[j] <= kProbTiny)
			c[j] = kProbTiny;

		for (int i = 0; i < 3; ++i)
			mixProb[i][j] = Pmark[i][j] * f.mu[i] / c[j];
	}

	// ---- 目标模型 1: CV 维度 2 ----
	for (int r = 0; r < 2; ++r)
	{
		mixX1[r] = 0.0;
		for (int c0 = 0; c0 < 2; ++c0)
			mixP1[r][c0] = 0.0;
	}
	for (int i = 0; i < 3; ++i)
	{
		mixX1[0] += mixProb[i][0] * xs[i][0];
		mixX1[1] += mixProb[i][0] * xs[i][1];
	}
	for (int i = 0; i < 3; ++i)
	{
		double Pi2[2][2];
		if (dims[i] == 2)
		{
			Pi2[0][0] = f.P1[0][0]; Pi2[0][1] = f.P1[0][1];
			Pi2[1][0] = f.P1[1][0]; Pi2[1][1] = f.P1[1][1];
		}
		else if (i == 1)
		{
			Pi2[0][0] = f.P2[0][0]; Pi2[0][1] = f.P2[0][1];
			Pi2[1][0] = f.P2[1][0]; Pi2[1][1] = f.P2[1][1];
		}
		else
		{
			Pi2[0][0] = f.P3[0][0]; Pi2[0][1] = f.P3[0][1];
			Pi2[1][0] = f.P3[1][0]; Pi2[1][1] = f.P3[1][1];
		}

		for (int r = 0; r < 2; ++r)
			for (int c0 = 0; c0 < 2; ++c0)
			{
				const double dr = xs[i][r] - mixX1[r];
				const double dc = xs[i][c0] - mixX1[c0];
				mixP1[r][c0] += mixProb[i][0] * (Pi2[r][c0] + dr * dc);
			}
	}
	positive_guard2_(mixP1);

	// ---- 目标模型 2/3: CA 维度 3 ----
	double* mixX[2] = { mixX2, mixX3 };
	double(*mixP[2])[3] = { mixP2, mixP3 };

	for (int dest = 0; dest < 2; ++dest)
	{
		const int jModel = dest + 1;
		for (int r = 0; r < 3; ++r)
		{
			mixX[dest][r] = 0.0;
			for (int c0 = 0; c0 < 3; ++c0)
				mixP[dest][r][c0] = 0.0;
		}

		for (int i = 0; i < 3; ++i)
		{
			mixX[dest][0] += mixProb[i][jModel] * xs[i][0];
			mixX[dest][1] += mixProb[i][jModel] * xs[i][1];
			mixX[dest][2] += mixProb[i][jModel] * ((dims[i] == 3) ? xs[i][2] : 0.0);
		}

		for (int i = 0; i < 3; ++i)
		{
			double Pi3[3][3] = { {0.0,0.0,0.0}, {0.0,0.0,0.0}, {0.0,0.0,0.0} };
			double xi3[3] = { xs[i][0], xs[i][1], (dims[i] == 3) ? xs[i][2] : 0.0 };
			if (dims[i] == 2)
			{
				Pi3[0][0] = f.P1[0][0]; Pi3[0][1] = f.P1[0][1];
				Pi3[1][0] = f.P1[1][0]; Pi3[1][1] = f.P1[1][1];
				Pi3[2][2] = (Meas_err_var > 0.0) ? 0.25 * Meas_err_var : 2500.0;
			}
			else if (i == 1)
			{
				for (int r = 0; r < 3; ++r)
					for (int c0 = 0; c0 < 3; ++c0)
						Pi3[r][c0] = f.P2[r][c0];
			}
			else
			{
				for (int r = 0; r < 3; ++r)
					for (int c0 = 0; c0 < 3; ++c0)
						Pi3[r][c0] = f.P3[r][c0];
			}

			for (int r = 0; r < 3; ++r)
				for (int c0 = 0; c0 < 3; ++c0)
				{
					const double dr = xi3[r] - mixX[dest][r];
					const double dc = xi3[c0] - mixX[dest][c0];
					mixP[dest][r][c0] += mixProb[i][jModel] * (Pi3[r][c0] + dr * dc);
				}
		}
		positive_guard3_(mixP[dest]);
	}
}

// ---------------- Kalman 单模型更新 ----------------
void RADAR_Kalman::predict_update_cv_(double x[2],
	double P[2][2],
	const double A[2][2],
	const double H[2],
	const double G[2],
	double q,
	double R,
	double z,
	double& likelihood)
{
	double xp[2];
	xp[0] = A[0][0] * x[0] + A[0][1] * x[1];
	xp[1] = A[1][0] * x[0] + A[1][1] * x[1];

	double AP[2][2];
	for (int r = 0; r < 2; ++r)
		for (int c = 0; c < 2; ++c)
			AP[r][c] = A[r][0] * P[0][c] + A[r][1] * P[1][c];

	double Pp[2][2];
	for (int r = 0; r < 2; ++r)
		for (int c = 0; c < 2; ++c)
			Pp[r][c] = AP[r][0] * A[c][0] + AP[r][1] * A[c][1] + q * G[r] * G[c];
	positive_guard2_(Pp);

	const double zPred = H[0] * xp[0] + H[1] * xp[1];
	const double y = z - zPred;

	const double PH0 = Pp[0][0] * H[0] + Pp[0][1] * H[1];
	const double PH1 = Pp[1][0] * H[0] + Pp[1][1] * H[1];
	double S = H[0] * PH0 + H[1] * PH1 + R;
	if (!std::isfinite(S) || S < kTiny)
		S = kTiny;

	const double K0 = PH0 / S;
	const double K1 = PH1 / S;

	x[0] = xp[0] + K0 * y;
	x[1] = xp[1] + K1 * y;

	// Joseph form
	double M[2][2];
	M[0][0] = 1.0 - K0 * H[0];
	M[0][1] = -K0 * H[1];
	M[1][0] = -K1 * H[0];
	M[1][1] = 1.0 - K1 * H[1];

	double MP[2][2];
	for (int r = 0; r < 2; ++r)
		for (int c = 0; c < 2; ++c)
			MP[r][c] = M[r][0] * Pp[0][c] + M[r][1] * Pp[1][c];

	for (int r = 0; r < 2; ++r)
		for (int c = 0; c < 2; ++c)
			P[r][c] = MP[r][0] * M[c][0] + MP[r][1] * M[c][1]
			+ R * ((r == 0) ? K0 : K1) * ((c == 0) ? K0 : K1);
	positive_guard2_(P);

	const double exponent = -0.5 * y * y / S;
	likelihood = (exponent < -690.0) ? kProbTiny : std::exp(exponent) / std::sqrt(std::max(S, kTiny));
	if (!std::isfinite(likelihood) || likelihood < kProbTiny)
		likelihood = kProbTiny;
}

void RADAR_Kalman::predict_update_ca_(double x[3],
	double P[3][3],
	const double A[3][3],
	const double H[3],
	const double G[3],
	double q,
	double R,
	double z,
	double& likelihood)
{
	double xp[3];
	for (int r = 0; r < 3; ++r)
		xp[r] = A[r][0] * x[0] + A[r][1] * x[1] + A[r][2] * x[2];

	double AP[3][3];
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			AP[r][c] = A[r][0] * P[0][c] + A[r][1] * P[1][c] + A[r][2] * P[2][c];

	double Pp[3][3];
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			Pp[r][c] = AP[r][0] * A[c][0] + AP[r][1] * A[c][1] + AP[r][2] * A[c][2]
			+ q * G[r] * G[c];
	positive_guard3_(Pp);

	const double zPred = H[0] * xp[0] + H[1] * xp[1] + H[2] * xp[2];
	const double y = z - zPred;

	double PH[3];
	for (int r = 0; r < 3; ++r)
		PH[r] = Pp[r][0] * H[0] + Pp[r][1] * H[1] + Pp[r][2] * H[2];

	double S = H[0] * PH[0] + H[1] * PH[1] + H[2] * PH[2] + R;
	if (!std::isfinite(S) || S < kTiny)
		S = kTiny;

	double K[3];
	for (int r = 0; r < 3; ++r)
		K[r] = PH[r] / S;

	for (int r = 0; r < 3; ++r)
		x[r] = xp[r] + K[r] * y;

	double M[3][3];
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			M[r][c] = ((r == c) ? 1.0 : 0.0) - K[r] * H[c];

	double MP[3][3];
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			MP[r][c] = M[r][0] * Pp[0][c] + M[r][1] * Pp[1][c] + M[r][2] * Pp[2][c];

	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			P[r][c] = MP[r][0] * M[c][0] + MP[r][1] * M[c][1] + MP[r][2] * M[c][2]
			+ R * K[r] * K[c];
	positive_guard3_(P);

	const double exponent = -0.5 * y * y / S;
	likelihood = (exponent < -690.0) ? kProbTiny : std::exp(exponent) / std::sqrt(std::max(S, kTiny));
	if (!std::isfinite(likelihood) || likelihood < kProbTiny)
		likelihood = kProbTiny;
}

// ---------------- 每轴 IMM 递推 ----------------
double RADAR_Kalman::process_axis_(AxisIMM& f, double z, int axis)
{
	double c[3];
	double mixX1[2], mixP1[2][2];
	double mixX2[3], mixP2[3][3];
	double mixX3[3], mixP3[3][3];

	mix_axis_(f, c, mixX1, mixP1, mixX2, mixP2, mixX3, mixP3);

	double A1[2][2], H1[2], G1[2], q1, R1;
	double A2[3][3], H2[3], G2[3], q2, R2;
	double A3[3][3], H3[3], G3[3], q3, R3;

	extract_cv_matrices_(axis, A1, H1, G1, q1, R1);
	extract_ca_matrices_(axis, q2_mat, q2_matSize, A2, H2, G2, q2, R2);
	extract_ca_matrices_(axis, q3_mat, q3_matSize, A3, H3, G3, q3, R3);

	double like[3];
	predict_update_cv_(mixX1, mixP1, A1, H1, G1, q1, R1, z, like[0]);
	predict_update_ca_(mixX2, mixP2, A2, H2, G2, q2, R2, z, like[1]);
	predict_update_ca_(mixX3, mixP3, A3, H3, G3, q3, R3, z, like[2]);

	double newMu[3];
	double sumMu = 0.0;
	for (int j = 0; j < 3; ++j)
	{
		newMu[j] = std::max(like[j], kProbTiny) * std::max(c[j], kTiny);
		if (!std::isfinite(newMu[j]) || newMu[j] < 0.0)
			newMu[j] = 0.0;
		sumMu += newMu[j];
	}

	if (sumMu <= kTiny)
	{
		newMu[0] = f.mu[0];
		newMu[1] = f.mu[1];
		newMu[2] = f.mu[2];
	}

	normalize_mu_(newMu);

	for (int i = 0; i < 2; ++i)
	{
		f.x1[i] = mixX1[i];
		for (int j = 0; j < 2; ++j)
			f.P1[i][j] = mixP1[i][j];
	}

	for (int i = 0; i < 3; ++i)
	{
		f.x2[i] = mixX2[i];
		f.x3[i] = mixX3[i];
		for (int j = 0; j < 3; ++j)
		{
			f.P2[i][j] = mixP2[i][j];
			f.P3[i][j] = mixP3[i][j];
		}
		f.mu[i] = newMu[i];
	}

	f.zPrev = z;
	return f.mu[0] * f.x1[0] + f.mu[1] * f.x2[0] + f.mu[2] * f.x3[0];
}

bool RADAR_Kalman::Setup()
{
	sampleIndex_ = 0ULL;
	reset_axis_(axisX_);
	reset_axis_(axisY_);

	double u0[3];
	get_u0_(u0);
	for (int i = 0; i < 3; ++i)
	{
		axisX_.mu[i] = u0[i];
		axisY_.mu[i] = u0[i];
	}
	return true;
}

bool RADAR_Kalman::Run()
{
	const double zx = x_in[0];
	const double zy = y_in[0];

	if (sampleIndex_ == 0ULL)
	{
		init_axis_from_first_sample_(axisX_, zx);
		init_axis_from_first_sample_(axisY_, zy);

		x_out[0] = zx;
		y_out[0] = zy;

		++sampleIndex_;
		return true;
	}

	if (sampleIndex_ == 1ULL)
	{
		init_axis_from_second_sample_(axisX_, zx);
		init_axis_from_second_sample_(axisY_, zy);

		x_out[0] = zx;
		y_out[0] = zy;

		++sampleIndex_;
		return true;
	}

	const double xEst = process_axis_(axisX_, zx, 0);
	const double yEst = process_axis_(axisY_, zy, 1);

	x_out[0] = xEst;
	y_out[0] = yEst;

	++sampleIndex_;
	return true;
}
