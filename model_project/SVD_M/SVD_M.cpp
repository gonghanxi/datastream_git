#include "SVD_M.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(SVD_M)
{
	SET_MODEL_DESCRIPTION("Singular Value Decomposition of a Toeplitz Matrix");
	SET_MODEL_SYMBOL("SYM_SVD_M");
	SET_MODEL_CATEGORY("Math Matrix");

	// Ports
	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("Input stream");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(svals);
		p.SetName("svals");
		p.SetDescription("The singular values of input - The diagonal of W");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(rsvec);
		p.SetName("rsvec");
		p.SetDescription("Right singular vectors of input - V");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(lsvec);
		p.SetName("lsvec");
		p.SetDescription("Left singular vectors of input - U");
	}

	// Parameters
	{
		DFParam p = ADD_MODEL_PARAM(Threshold);
		p.SetName("Threshold");
		p.SetDefaultValue("1e-17");
		p.SetDescription("Threshold for similarities (algorithm assumes values below Threshold have reached zero)");
	}
	{
		DFParam p = ADD_MODEL_PARAM(MaxIterations);
		p.SetName("MaxIterations");
		p.SetDefaultValue("30");
		p.SetDescription("Maximum iterations for SVD convergence");
	}
	{
		DFParam p = ADD_MODEL_ENUM_PARAM(m_GenerateLeft, SVD_M::GenerateLeftE);
		p.SetName("GenerateLeft");
		p.AddEnumeration("Do not Generate Left Singular Vectors", SVD_M::DoNotGenerateLeft);
		p.AddEnumeration("Generate Left Singular Vectors", SVD_M::GenerateLeft);
		p.SetDefaultValue("Generate Left Singular Vectors");
		p.SetDescription("Matrix generation of left singular vectors");
	}
	{
		DFParam p = ADD_MODEL_ENUM_PARAM(m_GenerateRight, SVD_M::GenerateRightE);
		p.SetName("GenerateRight");
		p.AddEnumeration("Do not Generate Right Singular Vectors", SVD_M::DoNotGenerateRight);
		p.AddEnumeration("Generate Right Singular Vectors", SVD_M::GenerateRight);
		p.SetDefaultValue("Generate Right Singular Vectors");
		p.SetDescription("Matrix generation of right singular vectors");
	}

	return true;
}
#endif // SV_CODE_GEN

// 固定使用你给的 Matrix.h：NumRows / NumColumns
namespace
{
	inline size_t rows_of(const SystemVueModelBuilder::Matrix<double>& m) { return m.NumRows(); }
	inline size_t cols_of(const SystemVueModelBuilder::Matrix<double>& m) { return m.NumColumns(); }

	inline double SIGN(double a, double b) { return (b >= 0.0) ? std::fabs(a) : -std::fabs(a); }

	inline void swap_cols(Matrix<double>& M, int c1, int c2)
	{
		const int r = (int)M.NumRows();
		for (int i = 0; i < r; ++i)
			std::swap(M(i, c1), M(i, c2));
	}

	inline void scale_col(Matrix<double>& M, int c, double s)
	{
		const int r = (int)M.NumRows();
		for (int i = 0; i < r; ++i)
			M(i, c) *= s;
	}

	inline double dot_col(const Matrix<double>& A, const Matrix<double>& B, int c)
	{
		const int r = (int)A.NumRows();
		double acc = 0.0;
		for (int i = 0; i < r; ++i) acc += A(i, c) * B(i, c);
		return acc;
	}

	// 选每列“绝对值最大的元素”为 pivot，规定 pivot 为正 => 确定性符号规范
	inline int pivot_row_maxabs(const Matrix<double>& M, int c)
	{
		const int r = (int)M.NumRows();
		int idx = 0;
		double best = 0.0;
		for (int i = 0; i < r; ++i)
		{
			double v = std::fabs(M(i, c));
			if (v > best) { best = v; idx = i; }
		}
		return idx;
	}
}

// ---------- ctor ----------
SVD_M::SVD_M()
	: Threshold(1e-17)
	, MaxIterations(30)
	, m_GenerateLeft(GenerateLeft)
	, m_GenerateRight(GenerateRight)
	, nrows(0)
	, ncols(0)
	, m_hasPrevV(false)
{
	// 这句放 ctor 里更保险（避免某些版本在 flatten/analysis 阶段读到未初始化 rate）
	input.SetRate(1);
	svals.SetRate(1);
	rsvec.SetRate(1);
	lsvec.SetRate(1);
}

// ---------- helpers ----------
double SVD_M::hypot(double a, double b)
{
	a = std::fabs(a);
	b = std::fabs(b);
	if (a > b)
	{
		double r = b / a;
		return a * std::sqrt(1.0 + r * r);
	}
	if (b > 0.0)
	{
		double r = a / b;
		return b * std::sqrt(1.0 + r * r);
	}
	return 0.0;
}

void SVD_M::transpose(const Matrix<double>& A, Matrix<double>& AT)
{
	const size_t r = rows_of(A);
	const size_t c = cols_of(A);
	AT.Resize(c, r);
	for (size_t i = 0; i < r; ++i)
		for (size_t j = 0; j < c; ++j)
			AT(j, i) = A(i, j);
}

// ---------- Initialize / Finalize ----------
bool SVD_M::Initialize()
{
	input.SetRate(1);
	svals.SetRate(1);
	rsvec.SetRate(1);
	lsvec.SetRate(1);

	if (MaxIterations < 1) MaxIterations = 1;
	if (Threshold < 0) Threshold = 0;

	m_hasPrevV = false;
	m_prevV.Resize(0, 0);

	return true;
}

bool SVD_M::Finalize()
{
	return true;
}

// ---------- Golub-Reinsch SVD (m>=n) ----------
void SVD_M::calc_svd(const Matrix<double>& A,
	Matrix<double>& Uo,
	Matrix<double>& Wo,
	Matrix<double>& Vo,
	double threshold,
	int maxIters,
	int needV)
{
	const int m = (int)rows_of(A);
	const int n = (int)cols_of(A);

	Uo.Resize(m, n);
	for (int i = 0; i < m; ++i)
		for (int j = 0; j < n; ++j)
			Uo(i, j) = A(i, j);

	std::vector<double> w(n, 0.0);
	std::vector<double> rv1(n, 0.0);

	if (needV) Vo.Resize(n, n);
	else       Vo.Resize(0, 0);

	double g = 0.0, scale = 0.0, anorm = 0.0;

	// Householder reduction
	for (int i = 0; i < n; ++i)
	{
		int l = i + 1;
		rv1[i] = scale * g;
		g = 0.0;
		double s = 0.0;
		scale = 0.0;

		if (i < m)
		{
			for (int k = i; k < m; ++k) scale += std::fabs(Uo(k, i));
			if (scale > 0.0)
			{
				for (int k = i; k < m; ++k)
				{
					Uo(k, i) /= scale;
					s += Uo(k, i) * Uo(k, i);
				}
				double f = Uo(i, i);
				g = -SIGN(std::sqrt(s), f);
				double h = f * g - s;
				Uo(i, i) = f - g;

				for (int j = l; j < n; ++j)
				{
					s = 0.0;
					for (int k = i; k < m; ++k) s += Uo(k, i) * Uo(k, j);
					double f2 = (h != 0.0) ? (s / h) : 0.0;
					for (int k = i; k < m; ++k) Uo(k, j) += f2 * Uo(k, i);
				}
				for (int k = i; k < m; ++k) Uo(k, i) *= scale;
			}
		}

		w[i] = scale * g;

		g = 0.0; s = 0.0; scale = 0.0;
		if (i < m && i != n - 1)
		{
			for (int k = l; k < n; ++k) scale += std::fabs(Uo(i, k));
			if (scale > 0.0)
			{
				for (int k = l; k < n; ++k)
				{
					Uo(i, k) /= scale;
					s += Uo(i, k) * Uo(i, k);
				}
				double f = Uo(i, l);
				g = -SIGN(std::sqrt(s), f);
				double h = f * g - s;
				Uo(i, l) = f - g;

				for (int k = l; k < n; ++k)
					rv1[k] = (h != 0.0) ? (Uo(i, k) / h) : 0.0;

				for (int j = l; j < m; ++j)
				{
					s = 0.0;
					for (int k = l; k < n; ++k) s += Uo(j, k) * Uo(i, k);
					for (int k = l; k < n; ++k) Uo(j, k) += s * rv1[k];
				}
				for (int k = l; k < n; ++k) Uo(i, k) *= scale;
			}
		}

		anorm = std::max(anorm, std::fabs(w[i]) + std::fabs(rv1[i]));
	}

	// Accumulate right-hand transformation
	if (needV)
	{
		for (int i = n - 1; i >= 0; --i)
		{
			int l = i + 1;
			g = rv1[i];
			if (i < n - 1)
			{
				if (g != 0.0)
				{
					for (int j = l; j < n; ++j)
						Vo(j, i) = (Uo(i, j) / Uo(i, l)) / g;

					for (int j = l; j < n; ++j)
					{
						double s2 = 0.0;
						for (int k = l; k < n; ++k) s2 += Uo(i, k) * Vo(k, j);
						for (int k = l; k < n; ++k) Vo(k, j) += s2 * Vo(k, i);
					}
				}
				for (int j = l; j < n; ++j) { Vo(i, j) = 0.0; Vo(j, i) = 0.0; }
			}
			Vo(i, i) = 1.0;
		}
	}

	// Accumulate left-hand transformation
	for (int i = n - 1; i >= 0; --i)
	{
		int l = i + 1;
		g = w[i];
		for (int j = l; j < n; ++j) Uo(i, j) = 0.0;

		if (g != 0.0)
		{
			double ginvt = 1.0 / g;
			for (int j = l; j < n; ++j)
			{
				double s2 = 0.0;
				for (int k = l; k < m; ++k) s2 += Uo(k, i) * Uo(k, j);
				double f = (Uo(i, i) != 0.0) ? ((s2 / Uo(i, i)) * ginvt) : 0.0;
				for (int k = i; k < m; ++k) Uo(k, j) += f * Uo(k, i);
			}
			for (int j = i; j < m; ++j) Uo(j, i) *= ginvt;
		}
		else
		{
			for (int j = i; j < m; ++j) Uo(j, i) = 0.0;
		}
		Uo(i, i) += 1.0;
	}

	// --- 关键：使用缩放后的 tol，避免 6x4 时 l==0 导致 l-1 越界/数值污染 ---
	const double eps = std::numeric_limits<double>::epsilon();
	const double tol = std::max(threshold * anorm, eps * anorm);

	// Diagonalize bidiagonal form
	for (int k = n - 1; k >= 0; --k)
	{
		for (int its = 0; its < maxIters; ++its)
		{
			int l = 0;
			bool flag = true;

			for (l = k; l >= 0; --l)
			{
				if (std::fabs(rv1[l]) <= tol) { flag = false; break; }
				if (l == 0) break;
				if (std::fabs(w[l - 1]) <= tol) break;
			}

			// 0-based 下防护：flag=true 且 l==0 会导致访问 l-1
			if (flag && l == 0) flag = false;

			if (flag)
			{
				double c = 0.0, s = 1.0;
				const int nm = l - 1; // 这里保证 l>0

				for (int i = l; i <= k; ++i)
				{
					double f = s * rv1[i];
					rv1[i] = c * rv1[i];
					if (std::fabs(f) <= tol) break;

					double g2 = w[i];
					double h = hypot(f, g2);
					w[i] = h;
					h = (h != 0.0) ? (1.0 / h) : 0.0;
					c = g2 * h;
					s = -f * h;

					for (int j = 0; j < m; ++j)
					{
						double y = Uo(j, nm);
						double z = Uo(j, i);
						Uo(j, nm) = y * c + z * s;
						Uo(j, i) = z * c - y * s;
					}
				}
			}

			double z = w[k];
			if (l == k)
			{
				if (z < 0.0)
				{
					w[k] = -z;
					if (needV)
						for (int j = 0; j < n; ++j) Vo(j, k) = -Vo(j, k);
				}
				break;
			}

			if (its == maxIters - 1)
			{
				POST_ERROR("SVD_M: no convergence within MaxIterations.");
			}

			int nm = k - 1;
			double x = w[l];
			double y = w[nm];
			double g2 = rv1[nm];
			double h = rv1[k];

			double denom = 2.0 * h * y;
			if (std::fabs(denom) < 1e-300) denom = (denom >= 0.0) ? 1e-300 : -1e-300;

			double f = ((y - z) * (y + z) + (g2 - h) * (g2 + h)) / denom;
			double g3 = hypot(f, 1.0);
			f = ((x - z) * (x + z) + h * (y / (f + SIGN(g3, f)) - h)) / (x + 1e-300);

			double c = 1.0, s = 1.0;
			for (int j = l; j <= nm; ++j)
			{
				int i = j + 1;
				double g = rv1[i];
				y = w[i];

				double h2 = s * g;
				g = c * g;

				double z2 = hypot(f, h2);
				rv1[j] = z2;
				c = (z2 != 0.0) ? (f / z2) : 1.0;
				s = (z2 != 0.0) ? (h2 / z2) : 0.0;

				f = x * c + g * s;
				g = g * c - x * s;
				h2 = y * s;
				y = y * c;

				if (needV)
				{
					for (int jj = 0; jj < n; ++jj)
					{
						double xx = Vo(jj, j);
						double zz = Vo(jj, i);
						Vo(jj, j) = xx * c + zz * s;
						Vo(jj, i) = zz * c - xx * s;
					}
				}

				z2 = hypot(f, h2);
				w[j] = z2;
				c = (z2 != 0.0) ? (f / z2) : 1.0;
				s = (z2 != 0.0) ? (h2 / z2) : 0.0;

				f = c * g + s * y;
				x = c * y - s * g;

				for (int jj = 0; jj < m; ++jj)
				{
					double yy2 = Uo(jj, j);
					double zz2 = Uo(jj, i);
					Uo(jj, j) = yy2 * c + zz2 * s;
					Uo(jj, i) = zz2 * c - yy2 * s;
				}
			}

			rv1[l] = 0.0;
			rv1[k] = f;
			w[k] = x;
		}
	}

	// --- 内置/常见实现：按奇异值降序排序，并同步交换 U/V 的列 ---
	for (int i = 0; i < n - 1; ++i)
	{
		int k = i;
		double p = w[i];
		for (int j = i + 1; j < n; ++j)
		{
			if (w[j] > p) { k = j; p = w[j]; }
		}
		if (k != i)
		{
			std::swap(w[i], w[k]);
			swap_cols(Uo, i, k);
			if (needV) swap_cols(Vo, i, k);
		}
	}

	// --- 确定性符号规范：pivot 为正（尽量贴近内置“习惯”且每帧稳定） ---
	if (needV)
	{
		for (int c = 0; c < n; ++c)
		{
			int pr = pivot_row_maxabs(Vo, c);
			if (Vo(pr, c) < 0.0)
			{
				scale_col(Vo, c, -1.0);
				scale_col(Uo, c, -1.0); // 保持 A=U*S*V^T 不变
			}
		}
	}
	else
	{
		for (int c = 0; c < n; ++c)
		{
			int pr = pivot_row_maxabs(Uo, c);
			if (Uo(pr, c) < 0.0)
				scale_col(Uo, c, -1.0);
		}
	}

	// Build diagonal W matrix (n×n)
	Wo.Resize(n, n);
	Wo.Zero();
	for (int i = 0; i < n; ++i)
		Wo(i, i) = (std::fabs(w[i]) < threshold) ? 0.0 : w[i];
}

// ---------- Run ----------
bool SVD_M::Run()
{
	const Matrix<double>& A = input[0];
	nrows = rows_of(A);
	ncols = cols_of(A);

	Matrix<double> S;   // svals output (vector)
	Matrix<double> Uout, Vout;

	if (nrows == 0 || ncols == 0)
	{
		svals[0].Resize(0, 0);
		rsvec[0].Resize(0, 0);
		lsvec[0].Resize(0, 0);
		return true;
	}

	const bool needLeft = (m_GenerateLeft == GenerateLeft);
	const bool needRight = (m_GenerateRight == GenerateRight);

	if (nrows >= ncols)
	{
		// A(m×n) = U(m×n) * W(n×n) * V(n×n)^T
		calc_svd(A, U, W, V, Threshold, MaxIterations, needRight ? 1 : 0);

		// svals: diag(W) as (n×1)
		S.Resize(ncols, 1);
		for (size_t i = 0; i < ncols; ++i)
			S((int)i, 0) = W((int)i, (int)i);

		if (needLeft)  Uout = U; else Uout.Resize(0, 0);
		if (needRight) Vout = V; else Vout.Resize(0, 0);

		// --- 跨帧符号稳定（只做 sign，不做旋转/置换）---
		if (needRight)
		{
			if (!m_hasPrevV || m_prevV.NumRows() != Vout.NumRows() || m_prevV.NumColumns() != Vout.NumColumns())
			{
				m_prevV = Vout;
				m_hasPrevV = true;
			}
			else
			{
				const int n = (int)Vout.NumColumns();
				for (int c = 0; c < n; ++c)
				{
					double d = dot_col(m_prevV, Vout, c);
					if (d < 0.0)
					{
						scale_col(Vout, c, -1.0);
						if (needLeft) scale_col(Uout, c, -1.0);
					}
				}
				m_prevV = Vout;
			}
		}
		else
		{
			// 关闭右奇异向量输出时，不做跨帧对齐，避免状态干扰
			m_hasPrevV = false;
			m_prevV.Resize(0, 0);
		}
	}
	else
	{
		// 你当前关注的是 m>=n；这里保持你原逻辑不动（避免引入新变量）
		Matrix<double> AT, U2, W2, V2;
		transpose(A, AT);

		calc_svd(AT, U2, W2, V2, Threshold, MaxIterations, /*needV*/ 1);

		// svals: diag(W2) as (m×1)
		S.Resize(nrows, 1);
		for (size_t i = 0; i < nrows; ++i)
			S((int)i, 0) = W2((int)i, (int)i);

		if (needLeft)
		{
			transpose(V2, Uout);
		}
		else
		{
			Uout.Resize(0, 0);
		}

		if (needRight)
		{
			transpose(U2, Vout);
		}
		else
		{
			Vout.Resize(0, 0);
		}

		m_hasPrevV = false;
		m_prevV.Resize(0, 0);
	}

	svals[0] = S;
	lsvec[0] = Uout;
	rsvec[0] = Vout;

	return true;
}
