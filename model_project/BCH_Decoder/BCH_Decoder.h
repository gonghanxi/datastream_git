#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <vector>
#include <algorithm>
#include <limits>

class SYSTEMVUEMODELBUILDER_API BCH_Decoder : public SystemVueModelBuilder::DFModel
{
public:
	enum EraseEnum
	{
		ERASE_NO = 0,
		ERASE_YES = 1
	};

	DECLARE_MODEL_INTERFACE(BCH_Decoder);
	BCH_Decoder();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<int> Code;
	SystemVueModelBuilder::CircularBuffer<int> EraseFlag;
	SystemVueModelBuilder::CircularBuffer<int> Msg;

	int   M;               
	int   K;               
	int   T;               
	int   CodeLength;      

	int* PrimPoly;
	int  PrimPolySize;

	EraseEnum Erase;       

	int* ErasePosition;
	int  ErasePositionSize;

    static int defaultPrimPolyInt(int m);
    void decodeCore(const std::vector<int> &r_in,
        std::vector<int>       &c_out,
        std::vector<int>       &msg_out);
private:
	int  N_;   
	int  Ns_;  
	int  Ks_;  
	bool eraseFlagConnected_;

	std::vector<int> alpha_to_;
	std::vector<int> index_of_;



	int  parsePrimitivePolynomial() const;

	void buildField();

	inline int gf_add(int a, int b) const { return a ^ b; } 
	int  gf_mul(int a, int b) const; 
	int  gf_div(int a, int b) const; 


};
