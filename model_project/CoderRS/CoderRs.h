#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <vector>

class CoderRS : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(CoderRS);
	CoderRS();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<int> in;   
	SystemVueModelBuilder::CircularBuffer<int> out;  

	int   GF;             
	int   CodeLength;     
	int   MessageLength;  

	int*  PrimPoly;       
	int   PrimPolySize;   

	int   Root;           


	int m_;          
	int fieldSize_;  
	int fieldMask_;  
	int maxExp_;     

	int n_;          
	int k_;          

	std::vector<int> alpha_to_;   
	std::vector<int> index_of_;   

	std::vector<int> g_;

	void buildField();      
	void buildGenerator();  

	int  gf_add(int a, int b) const; 
	int  gf_mul(int a, int b) const; 
};
