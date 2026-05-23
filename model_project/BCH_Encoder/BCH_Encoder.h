#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <vector>
#include <algorithm>

class SYSTEMVUEMODELBUILDER_API BCH_Encoder : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(BCH_Encoder);
	BCH_Encoder();

	bool Setup() override;
	bool Run()   override;

	// --------- ¶Ë¿Ú ---------
	SystemVueModelBuilder::CircularBuffer<int> Msg;
	SystemVueModelBuilder::CircularBuffer<int> Code;

	int M;          
	int K;          
	int MsgLength;  

	int* GenPoly;
	int  GenPolySize;


	int N_;          
	int Ks_;         
	int Ns_;         
	int parityLen_;  

	std::vector<int> g_;

	void buildGenerator();

	void encodeOne(const std::vector<int>& u, std::vector<int>& c_out);
};
