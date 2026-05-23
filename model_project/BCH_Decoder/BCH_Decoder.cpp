#include "BCH_Decoder.h"

#include <stdexcept>
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(BCH_Decoder)
{
	SET_MODEL_DESCRIPTION("Binary primitive BCH decoder");
	SET_MODEL_SYMBOL("SYM_BCH_Decoder");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(Code);
		p.SetDescription("Uncoded binary codeword");
	}
	{
		auto p = ADD_MODEL_INPUT(EraseFlag);
		p.SetDescription("Specifying the corresponding bits is erased (1) or not (0)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_OUTPUT(Msg);
		p.SetDescription("Decoded binary messageword");
	}

	{
		auto p = ADD_MODEL_PARAM(M);
		p.SetDefaultValue("3");
		p.SetDescription("The root of generation polynomial is defined in Gf(2^M), primitive codeword length N=2^M-1");
	}
	{
		auto p = ADD_MODEL_PARAM(K);
		p.SetDefaultValue("4");
		p.SetDescription("Primitive message length (Unshortened)");
	}
	{
		auto p = ADD_MODEL_PARAM(T);
		p.SetDefaultValue("1");
		p.SetDescription("Error correction capability");
	}
	{
		auto p = ADD_MODEL_PARAM(CodeLength);
		p.SetDefaultValue("0");
		p.SetDescription("Shortened codeword length(<=2^M-1 ), set 0 for unshortened code");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(PrimPoly, PrimPolySize);
		p.SetDefaultValue("[]");
		p.SetDescription("primitive polynomial in the form of integer (default []), binary vector or power vector of non-zero item");
	}
	{
		auto p = ADD_MODEL_ENUM_PARAM(Erase, EraseEnum);
		p.AddEnumeration("NO", BCH_Decoder::ERASE_NO);
		p.AddEnumeration("YES", BCH_Decoder::ERASE_YES);
		p.SetDefaultValue("NO");
		p.SetDescription("there is erased bits or not in undecoded code");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(ErasePosition, ErasePositionSize);
		p.SetDefaultValue("[]");
		p.SetDescription("index array of erased bits (from [0,Codelength-1]), valid if Erase==YES and pin EraseFlag is disconnected");
		p.SetHideCondition("Erase ~= 1");
	}

	return true;
}
#endif 

BCH_Decoder::BCH_Decoder()
	: M(3)
	, K(4)
	, T(1)
	, CodeLength(0)
	, PrimPoly(nullptr)
	, PrimPolySize(0)
	, Erase(ERASE_NO)
	, ErasePosition(nullptr)
	, ErasePositionSize(0)
	, N_(0)
	, Ns_(0)
	, Ks_(0)
	, eraseFlagConnected_(false)
{
}

bool BCH_Decoder::Setup()
{
	eraseFlagConnected_ = EraseFlag.IsConnected();

	if (M < 1)  M = 1;
	if (M > 20) M = 20;

	N_ = (1 << M) - 1;

	int effectiveCodeLength = CodeLength;
	if (effectiveCodeLength <= 0 || effectiveCodeLength > N_)
		effectiveCodeLength = N_;
	Ns_ = std::min(N_, effectiveCodeLength);

	if (CodeLength <= 0 || CodeLength >= N_)
		Ks_ = K;
	else
		Ks_ = K + (CodeLength - N_);

	if (Ks_ < 0)   Ks_ = 0;
	if (Ks_ > Ns_) Ks_ = Ns_;

	Code.SetRate((unsigned)Ns_);
	Msg.SetRate((unsigned)Ks_);
	if (eraseFlagConnected_)
		EraseFlag.SetRate((unsigned)Ns_);

	buildField();
	return true;
}


int BCH_Decoder::defaultPrimPolyInt(int m)
{
	static const int tbl[21] = {
		0,
		3,       // 1
		7,       // 2
		11,      // 3
		19,      // 4
		37,      // 5
		67,      // 6
		137,     // 7
		285,     // 8
		529,     // 9
		1033,    // 10
		2053,    // 11
		4179,    // 12
		8219,    // 13
		17475,   // 14
		32771,   // 15
		69643,   // 16
		131081,  // 17
		262273,  // 18
		524327,  // 19
		1048585  // 20
	};

	if (m < 1)  m = 1;
	if (m > 20) m = 20;
	return tbl[m];
}


int BCH_Decoder::parsePrimitivePolynomial() const
{
	if (PrimPolySize <= 0 || PrimPoly == nullptr)
		return defaultPrimPolyInt(M);

	int polyInt = 0;

	if (PrimPolySize == 1)
	{
		polyInt = PrimPoly[0];
	}
	else
	{
		bool allZeroOne = true;
		for (int i = 0; i < PrimPolySize; ++i)
		{
			int v = PrimPoly[i];
			if (v < 0 || v > 1)
			{
				allZeroOne = false;
				break;
			}
		}

		if (allZeroOne)
		{
			for (int i = 0; i < PrimPolySize; ++i)
			{
				if (PrimPoly[i] & 1)
					polyInt |= (1 << i);
			}
		}
		else
		{
			for (int i = 0; i < PrimPolySize; ++i)
			{
				int e = PrimPoly[i];
				if (e >= 0 && e <= M)
					polyInt |= (1 << e);
			}
		}
	}

	if (polyInt == 0)
		polyInt = defaultPrimPolyInt(M);

	if ((polyInt & (1 << M)) == 0)
		polyInt |= (1 << M);
	if ((polyInt & 1) == 0)
		polyInt |= 1;

	return polyInt;
}

void BCH_Decoder::buildField()
{
	const int poly = parsePrimitivePolynomial();

	alpha_to_.assign(N_ + 1, 0);
	index_of_.assign((1 << M), 0);

	alpha_to_[0] = 1;

	for (int i = 1; i < N_; ++i)
	{
		int tmp = alpha_to_[i - 1] << 1;
		if (tmp & (1 << M))
			tmp ^= poly;
		alpha_to_[i] = tmp;
	}
	alpha_to_[N_] = 1;

	for (int i = 0; i < N_; ++i)
		index_of_[alpha_to_[i]] = i;
	index_of_[0] = -1;
}

int BCH_Decoder::gf_mul(int a, int b) const
{
	if (a == 0 || b == 0)
		return 0;

	int loga = index_of_[a];
	int logb = index_of_[b];
	int logc = loga + logb;
	if (logc >= N_)
		logc -= N_;
	return alpha_to_[logc];
}

int BCH_Decoder::gf_div(int a, int b) const
{
	if (a == 0)
		return 0;
	if (b == 0)
		return 0;

	int loga = index_of_[a];
	int logb = index_of_[b];
	int logc = loga - logb;
	if (logc < 0)
		logc += N_;
	return alpha_to_[logc];
}

void BCH_Decoder::decodeCore(const std::vector<int> &r_in,
	std::vector<int>       &c_out,
	std::vector<int>       &msg_out)
{
	const int Ns_run = static_cast<int>(r_in.size());
	if (Ns_run <= 0)
	{
		c_out.clear();
		msg_out.clear();
		return;
	}

	int Ks_run = Ks_;
	if (Ks_run > Ns_run)
		Ks_run = Ns_run;

	std::vector<int> r = r_in;

	if (T <= 0 || Ks_run <= 0)
	{
		c_out = r;
		msg_out.assign(r.begin(), r.begin() + Ks_run);
		return;
	}

	const int twoT = 2 * T;

	std::vector<int> S(twoT + 1, 0);
	bool allZero = true;

	for (int i = 1; i <= twoT; ++i)
	{
		int x = alpha_to_[i % N_];

		int s = r[0] ? 1 : 0;

		for (int j = 1; j < Ns_run; ++j)
		{
			if (s != 0)
				s = gf_mul(s, x);
			if (r[j] & 1)
				s ^= 1;
		}

		S[i] = s;
		if (s != 0)
			allZero = false;
	}

	if (allZero)
	{
		c_out = r;
		msg_out.assign(r.begin(), r.begin() + Ks_run);
		return;
	}

	const int maxDeg = twoT;

	std::vector<int> sigma(maxDeg + 1, 0);
	std::vector<int> B(maxDeg + 1, 0);
	std::vector<int> Ttmp(maxDeg + 1, 0);

	sigma[0] = 1;
	B[0] = 1;

	int L = 0;
	int m = 1;
	int b = 1;

	for (int n = 0; n < twoT; ++n)
	{
		int d = S[n + 1];
		for (int i = 1; i <= L; ++i)
		{
			if (sigma[i] != 0 && S[n + 1 - i] != 0)
				d ^= gf_mul(sigma[i], S[n + 1 - i]);
		}

		if (d == 0)
		{
			++m;
		}
		else if (2 * L <= n)
		{
			Ttmp = sigma;

			int factor = gf_div(d, b);
			for (int i = 0; i <= maxDeg; ++i)
			{
				if (B[i] != 0)
				{
					int idx = i + m;
					if (idx <= maxDeg)
						sigma[idx] ^= gf_mul(factor, B[i]);
				}
			}

			L = n + 1 - L;
			B = Ttmp;
			b = d;
			m = 1;
		}
		else
		{
			int factor = gf_div(d, b);
			for (int i = 0; i <= maxDeg; ++i)
			{
				if (B[i] != 0)
				{
					int idx = i + m;
					if (idx <= maxDeg)
						sigma[idx] ^= gf_mul(factor, B[i]);
				}
			}
			++m;
		}
	}

	std::vector<int> e(Ns_run, 0);

	for (int i = 0; i < Ns_run; ++i)
	{
		int i_prime = (N_ - Ns_run) + i;
		if (i_prime < 0)
			i_prime += N_;

		int exp = (i_prime + 1) % N_;
		int z = alpha_to_[exp];

		int sum = sigma[0];
		int zPow = 1;

		for (int k = 1; k <= L; ++k)
		{
			zPow = gf_mul(zPow, z);
			if (sigma[k] != 0 && zPow != 0)
				sum ^= gf_mul(sigma[k], zPow);
		}

		if (sum == 0)
			e[i] = 1;
	}

	for (int i = 0; i < Ns_run; ++i)
	{
		if (e[i])
			r[i] ^= 1;
	}

	c_out = r;
	msg_out.assign(r.begin(), r.begin() + Ks_run);
}

bool BCH_Decoder::Run()
{
	const int Ns_run = Ns_;   
	if (Ns_run <= 0)
		return true;

	std::vector<int> r_raw(Ns_run, 0);
	for (int i = 0; i < Ns_run; ++i)
		r_raw[i] = (Code[i] != 0) ? 1 : 0;

	std::vector<int> decodedMsg;

	if (Erase == ERASE_NO)
	{
		std::vector<int> c;
		decodeCore(r_raw, c, decodedMsg);
	}
	else
	{
		std::vector<int>  erasures;
		std::vector<char> isErased(Ns_run, 0);

		if (eraseFlagConnected_)
		{
			for (int i = 0; i < Ns_run; ++i)
			{
				if (EraseFlag[i] != 0)
				{
					erasures.push_back(i);
					isErased[i] = 1;
				}
			}
		}
		else
		{
			if (ErasePosition && ErasePositionSize > 0)
			{
				for (int k = 0; k < ErasePositionSize; ++k)
				{
					int pos = ErasePosition[k];
					if (pos >= 0 && pos < Ns_run)
					{
						erasures.push_back(pos);
						isErased[pos] = 1;
					}
				}
			}
		}

		if (T <= 0 || (int)erasures.size() > 2 * T)
		{
			std::vector<int> c;
			decodeCore(r_raw, c, decodedMsg);
		}
		else
		{
			std::vector<int> r0 = r_raw;
			std::vector<int> r1 = r_raw;
			for (int pos : erasures)
			{
				if (pos >= 0 && pos < Ns_run)
				{
					r0[pos] = 0;
					r1[pos] = 1;
				}
			}

			std::vector<int> c0, m0, c1, m1;
			decodeCore(r0, c0, m0);
			decodeCore(r1, c1, m1);

			auto distanceExcludingErased =
				[&](const std::vector<int> &c) -> int
			{
				int d = 0;
				for (int i = 0; i < Ns_run; ++i)
				{
					if (!isErased[i] && c[i] != r_raw[i])
						++d;
				}
				return d;
			};

			int d0 = distanceExcludingErased(c0);
			int d1 = distanceExcludingErased(c1);

			decodedMsg = (d0 <= d1) ? m0 : m1;
		}
	}

	int Ks_run = Ks_;
	if (Ks_run > (int)decodedMsg.size())
		Ks_run = static_cast<int>(decodedMsg.size());

	for (int i = 0; i < Ks_run; ++i)
		Msg[i] = decodedMsg[i] ? 1 : 0;
	for (int i = Ks_run; i < Ks_; ++i)
		Msg[i] = 0;

	return true;
}
