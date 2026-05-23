#include "BCH_Encoder.h"

#include <cmath>
#include <cstring>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(BCH_Encoder)
{
	SET_MODEL_DESCRIPTION("Binary BCH Encoder");
	SET_MODEL_SYMBOL("SYM_BCH_Encoder");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(Msg);
		p.SetDescription("Uncoded binary message");
	}
	{
		auto p = ADD_MODEL_OUTPUT(Code);
		p.SetDescription("Encoded binary codeword");
	}

	{
		auto p = ADD_MODEL_PARAM(M);
		p.SetDefaultValue("3");
		p.SetDescription("The root of generation polynomial is defined in GF(2^M), primitive codeword length N=2^M-1");
	}

	{
		auto p = ADD_MODEL_PARAM(K);
		p.SetDefaultValue("4");
		p.SetDescription("Primitive message length (Unshortened)");
	}

	{
		auto p = ADD_MODEL_PARAM(MsgLength);
		p.SetDefaultValue("0");
		p.SetDescription("Shortened message length [0,K], set 0 (or K) for unshortened code");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(GenPoly, GenPolySize);
		p.SetDefaultValue("[0,1,3]");
		p.SetDescription("Code generation polynomial (if g(x)=1+x+X^3, GenPoly=[0,1,3] or [1,1,0,1])");
	}

	return true;
}
#endif 

BCH_Encoder::BCH_Encoder()
	: M(3)
	, K(4)
	, MsgLength(0)
	, GenPoly(nullptr)
	, GenPolySize(0)
	, N_(0)
	, Ks_(0)
	, Ns_(0)
	, parityLen_(0)
{
}


void BCH_Encoder::buildGenerator()
{
	if (M < 1)  M = 1;
	if (M > 20) M = 20;

	N_ = (1 << M) - 1;
	parityLen_ = N_ - K;
	if (parityLen_ < 0)
		parityLen_ = 0;

	g_.assign(parityLen_ + 1, 0);

	if (GenPolySize <= 0 || GenPoly == nullptr)
	{
		if (parityLen_ >= 3)
		{
			g_[0] = 1;
			g_[1] = 1;
			g_[3] = 1;
		}
		else
		{
			g_[0] = 1;
			g_[parityLen_] = 1;
		}
		return;
	}

	bool allZeroOne = true;
	for (int i = 0; i < GenPolySize; ++i)
	{
		if (GenPoly[i] != 0 && GenPoly[i] != 1)
		{
			allZeroOne = false;
			break;
		}
	}

	if (allZeroOne && GenPolySize == parityLen_ + 1)
	{
		for (int i = 0; i <= parityLen_; ++i)
			g_[i] = GenPoly[i] & 1;

		if (g_[0] == 0)          g_[0] = 1;
		if (g_[parityLen_] == 0) g_[parityLen_] = 1;
		return;
	}

	if (allZeroOne && GenPolySize % (M + 1) == 0)
	{
		const int rows = GenPolySize / (M + 1);

		std::vector<int> poly(1, 1); 

		for (int r = 0; r < rows; ++r)
		{
			std::vector<int> f(M + 1, 0);
			for (int c = 0; c <= M; ++c)
			{
				f[c] = GenPoly[r * (M + 1) + c] & 1;
			}

			std::vector<int> prod(std::min((int)poly.size() + M, parityLen_ + 1), 0);

			for (int i = 0; i < (int)poly.size(); ++i)
			{
				if (!poly[i]) continue;
				for (int j = 0; j <= M; ++j)
				{
					if (!f[j]) continue;
					int deg = i + j;
					if (deg <= parityLen_)
						prod[deg] ^= 1;
				}
			}

			poly.swap(prod);
		}

		for (int i = 0; i <= parityLen_ && i < (int)poly.size(); ++i)
			g_[i] = poly[i] & 1;

		if (g_[0] == 0)          g_[0] = 1;
		if (g_[parityLen_] == 0) g_[parityLen_] = 1;
		return;
	}

	for (int i = 0; i < GenPolySize; ++i)
	{
		int e = GenPoly[i];
		if (e >= 0 && e <= parityLen_)
			g_[e] ^= 1; 
	}

	if (g_[0] == 0)          g_[0] = 1;
	if (g_[parityLen_] == 0) g_[parityLen_] = 1;
}


bool BCH_Encoder::Setup()
{
	buildGenerator();

	N_ = (1 << M) - 1;

	int effMsgLen = MsgLength;
	if (effMsgLen <= 0 || effMsgLen > K)
		effMsgLen = K;  

	Ks_ = std::min(K, effMsgLen);

	int delta = effMsgLen - K;       
	Ns_ = N_ + std::min(0, delta);

	if (Ks_ < 0)   Ks_ = 0;
	if (Ns_ < Ks_) Ns_ = Ks_;

	Msg.SetRate((unsigned)Ks_);
	Code.SetRate((unsigned)Ns_);

	return true;
}


void BCH_Encoder::encodeOne(const std::vector<int>& u, std::vector<int>& c_out)
{
	const int Ks_run = (int)u.size();
	const int r = parityLen_;           // N-K
	const int Ns_run = Ks_run + r;

	if (Ks_run <= 0 || r <= 0)
	{
		c_out = u;
		return;
	}

	std::vector<int> a(Ns_run, 0);

	for (int i = 0; i < Ks_run; ++i)
	{
		const int deg = r + (Ks_run - 1 - i); 
		a[deg] = u[i] & 1;
	}

	std::vector<int> tmp = a;

	for (int d = Ns_run - 1; d >= r; --d)
	{
		if (tmp[d] == 0)
			continue;

		const int shift = d - r;
		for (int j = 0; j <= r; ++j)
		{
			if (g_[j])
				tmp[shift + j] ^= 1;
		}
	}

	std::vector<int> b(r, 0);
	for (int i = 0; i < r; ++i)
		b[i] = tmp[r - 1 - i] & 1;  

	c_out.resize(Ns_run);
	for (int i = 0; i < Ks_run; ++i)
		c_out[i] = u[i] & 1;
	for (int i = 0; i < r; ++i)
		c_out[Ks_run + i] = b[i] & 1;
}


bool BCH_Encoder::Run()
{
	if (Ks_ <= 0 || Ns_ <= 0)
	{
		Code[0] = (Ks_ > 0 && Msg[0] != 0) ? 1 : 0;
		return true;
	}

	std::vector<int> u(Ks_, 0);
	for (int i = 0; i < Ks_; ++i)
		u[i] = (Msg[i] != 0) ? 1 : 0;

	std::vector<int> c;
	encodeOne(u, c);

	const int Ns_run = std::min(Ns_, (int)c.size());
	for (int i = 0; i < Ns_run; ++i)
		Code[i] = c[i] ? 1 : 0;

	for (int i = Ns_run; i < Ns_; ++i)
		Code[i] = 0;

	return true;
}
