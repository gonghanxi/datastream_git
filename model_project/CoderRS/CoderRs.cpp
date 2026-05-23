#include "CoderRs.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(CoderRS)
{
	SET_MODEL_DESCRIPTION("Reed Solomon Encoder");
	SET_MODEL_SYMBOL("SYM_CoderRS");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(in);
		p.SetName("in");
		p.SetDescription("information symbol");
	}
	{
		auto p = ADD_MODEL_OUTPUT(out);
		p.SetName("out");
		p.SetDescription("systematical code");
	}

	{
		auto p = ADD_MODEL_PARAM(GF);
		p.SetName("GF");
		p.SetDefaultValue("8");
		p.SetDescription("Galois field size (2^GF)");
	}

	{
		auto p = ADD_MODEL_PARAM(CodeLength);
		p.SetName("CodeLength");
		p.SetDefaultValue("255");
		p.SetDescription("Length of output codeword");
	}

	{
		auto p = ADD_MODEL_PARAM(MessageLength);
		p.SetName("MessageLength");
		p.SetDefaultValue("223");
		p.SetDescription("Length of input message symbols");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(PrimPoly, PrimPolySize);
		p.SetName("PrimPoly");
		p.SetDefaultValue("[1,0,1,1,1,0,0,0,1]");
		p.SetDescription(
			"Coefficients of primitive polynomial. "
			"PrimPoly must be the coefficients of the m order of polynomial");
	}

	{
		auto p = ADD_MODEL_PARAM(Root);
		p.SetName("Root");
		p.SetDefaultValue("1");
		p.SetDescription("First root of generator polynomial");
	}

	return true;
}
#endif 


CoderRS::CoderRS()
	: GF(8)
	, CodeLength(255)
	, MessageLength(223)
	, PrimPoly(nullptr)
	, PrimPolySize(0)
	, Root(1)
	, m_(8)
	, fieldSize_(0)
	, fieldMask_(0)
	, maxExp_(0)
	, n_(0)
	, k_(0)
{
}


bool CoderRS::Setup()
{
    LOG_INFO("CoderRS Setup FULL: set n,k, rates, buildField, buildGenerator.");

	n_ = CodeLength;
	k_ = MessageLength;

	if (n_ < 3)
		n_ = 3;
	if (k_ < 1)
		k_ = 1;
	if (k_ > n_ - 2)
		k_ = n_ - 2;        

	int maxN = (1 << GF) - 1;
	if (n_ > maxN)
		n_ = maxN;
	if (k_ > n_ - 2)
		k_ = n_ - 2;

	in.SetRate(static_cast<unsigned>(k_));
	out.SetRate(static_cast<unsigned>(n_));

	buildField();

	buildGenerator();

	return true;
}


int CoderRS::gf_add(int a, int b) const
{
	return a ^ b;
}


int CoderRS::gf_mul(int a, int b) const
{
	if (a == 0 || b == 0)
		return 0;

	int ia = index_of_[a];
	int ib = index_of_[b];

	if (ia < 0 || ib < 0)
		return 0;

	int ie = ia + ib;
	if (ie >= maxExp_)
		ie -= maxExp_;     // ģ (2^m - 1)

	return alpha_to_[ie];
}


void CoderRS::buildField()
{
	m_ = GF;
	if (m_ < 2)  m_ = 2;
	if (m_ > 16) m_ = 16;      

	fieldSize_ = 1 << m_;
	fieldMask_ = fieldSize_ - 1;
	maxExp_ = fieldSize_ - 1;

	bool useUserPoly = false;
	int  primPolyMask = 0;    
	bool highestOk = false;
	bool constantOk = false;

	if (PrimPoly != nullptr && PrimPolySize > 0)
	{
		int highestIdx = -1;
		for (int i = 0; i < PrimPolySize; ++i)
		{
			if (PrimPoly[i] != 0)
				highestIdx = i;
		}

		if (highestIdx == m_)
		{
			highestOk = true;

			if ((PrimPoly[0] & 1) != 0)
				constantOk = true;

			int cm = (m_ < PrimPolySize) ? PrimPoly[m_] : 0;
			if ((cm & 1) == 0)
				highestOk = false;

			if (highestOk && constantOk)
			{
				primPolyMask = 0;
				for (int i = 0; i < m_; ++i)
				{
					if (i < PrimPolySize && (PrimPoly[i] & 1))
						primPolyMask |= (1 << i);
				}

				primPolyMask &= fieldMask_;
				useUserPoly = true;
			}
		}
	}

	if (useUserPoly)
	{
        LOG_INFO("CoderRS buildField_v3: using user PrimPoly.");
	}
	else
	{
		switch (m_)
		{
		case 2:  primPolyMask = 0x7;    break;      // x^2 + x + 1
		case 3:  primPolyMask = 0xB;    break;      // x^3 + x + 1
		case 4:  primPolyMask = 0x13;   break;      // x^4 + x + 1
		case 5:  primPolyMask = 0x25;   break;      // x^5 + x^2 + 1
		case 6:  primPolyMask = 0x43;   break;      // x^6 + x + 1
		case 7:  primPolyMask = 0x89;   break;      // x^7 + x^3 + 1
		case 8:  primPolyMask = 0x11D;  break;      // x^8 + x^4 + x^3 + x^2 + 1
		default:
			primPolyMask = (1 << m_) | (1 << 1) | 1; // x^m + x + 1
			break;
		}

		primPolyMask &= fieldMask_;

        LOG_WARN("CoderRS buildField_v3: PrimPoly invalid or not set, using default primitive polynomial.");
	}

	alpha_to_.assign(maxExp_ + 1, 0);
	index_of_.assign(fieldSize_, -1);

	int alpha = 1;
	for (int i = 0; i < maxExp_; ++i)
	{
		alpha_to_[i] = alpha; 
		index_of_[alpha] = i;    

		alpha <<= 1;             
		if (alpha & fieldSize_)  
			alpha ^= primPolyMask;
		alpha &= fieldMask_;
	}

	alpha_to_[maxExp_] = 1;
	index_of_[0] = -1;       

    LOG_INFO("CoderRS buildField_v3: done.");
}

void CoderRS::buildGenerator()
{
	const int n = n_;
	const int k = k_;

	int parity = n - k;          
	if (parity <= 0)
	{
		g_.assign(1, 1);
        LOG_INFO("CoderRS buildGenerator: parity <= 0, use g(x)=1.");
		return;
	}

	int root = Root;
	if (root < 0)
		root = 0;
	if (maxExp_ > 0)
		root %= maxExp_;         

	g_.clear();
	g_.push_back(1);              

	for (int i = 0; i < parity; ++i)
	{
		int exp_i = root + i;
		while (exp_i >= maxExp_)
			exp_i -= maxExp_;

		int alpha_i = alpha_to_[exp_i];

		int deg = static_cast<int>(g_.size()) - 1;
		std::vector<int> new_g(deg + 2, 0);  

		for (int j = 0; j <= deg; ++j)
		{
			new_g[j + 1] ^= g_[j];
		}

		for (int j = 0; j <= deg; ++j)
		{
			if (g_[j] != 0)
			{
				int prod = gf_mul(g_[j], alpha_i);
				new_g[j] ^= prod;
			}
		}

		g_.swap(new_g);
	}

	if (static_cast<int>(g_.size()) > parity + 1)
		g_.resize(parity + 1);

    LOG_INFO("CoderRS buildGenerator: done.");
}

bool CoderRS::Run()
{
	const int parity = n_ - k_;
	if (parity <= 0)
	{
		for (int i = 0; i < k_; ++i)
			out[i] = in[i];
		return true;
	}

	std::vector<int> p(parity, 0);

	for (int i = 0; i < k_; ++i)
	{
		int sym = in[i] & fieldMask_;             
		int feedback = gf_add(sym, p[parity - 1]);

		for (int j = parity - 1; j > 0; --j)
		{
			if (feedback != 0 && g_[j] != 0)
			{
				p[j] = gf_add(p[j - 1], gf_mul(feedback, g_[j]));
			}
			else
			{
				p[j] = p[j - 1];
			}
		}

		if (feedback != 0 && g_[0] != 0)
			p[0] = gf_mul(feedback, g_[0]);
		else
			p[0] = 0;
	}

	for (int i = 0; i < k_; ++i)
		out[i] = in[i];

	for (int j = 0; j < parity; ++j)
		out[k_ + j] = p[parity - 1 - j];

	return true;
}
