#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVueModels.h"

#include <cstdint>

//#ifndef SYSTEMVUEMODELS_API
//#ifdef _WIN32
//#ifdef SYSTEMVUEMODELS_EXPORTS
//#define SYSTEMVUEMODELS_API __declspec(dllexport)
//#else
//#define SYSTEMVUEMODELS_API __declspec(dllimport)
//#endif
//#else
//#define SYSTEMVUEMODELS_API __attribute__((visibility("default")))
//#endif
//#endif

namespace SystemVueModelBuilder
{
    class  ConvolutionalCoder : public DFModel
	{
	public:
		BoolCircularBuffer m_cbInput;
		BoolCircularBuffer m_cbOutput;

		enum CodingRateEnum
		{
			rate_1_2 = 0,
			rate_1_3,
			rate_1_4,
			rate_1_5,
			rate_1_6,
			rate_1_7,
			rate_1_8
		};

		enum ZeroTailEnum
		{
			NO = 0,
			YES = 1
		};

		CodingRateEnum CodingRate;      
		int           ConstraintLength; 

		int* Polynomial;                
		int  PolynomialSize;

		ZeroTailEnum ZeroTail;          
		int         BitSequenceLength;  

		DECLARE_MODEL_INTERFACE(ConvolutionalCoder);

		ConvolutionalCoder();
		virtual ~ConvolutionalCoder() = default;

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;
		bool Finalize() override;


		int m_Counter;
		int m_inputFrmLen;   
		int m_currentState;  

		int      m_convoCodeRateN; 
		int      m_constraintLenK; 
		uint32_t m_regMaskK;       
		uint32_t m_polyMask[8];   

		int  BoundaryCheck(char functionTag);
		int  BitReverse(int mask, int constraintLen);
		static int rateToN(CodingRateEnum r);
		static int parity_u32(uint32_t v);
	};
}
