#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVueModels.h"

#include <cstdint>

namespace SystemVueModelBuilder
{
    class CRC_Decoder : public DFModel
	{
	public:
		CircularBuffer<bool> In;      
		CircularBuffer<bool> Out;     
		CircularBuffer<int>  Parity;  

		enum ParityPositionEnum { Tail = 0, Head = 1 };
		enum YesNoEnum { NO = 0, YES = 1 };

		ParityPositionEnum ParityPosition; 
		YesNoEnum ReverseData;             
		YesNoEnum ReverseParity;           
		YesNoEnum ComplementParity;        
		int MessageLength;                 
		int InitialState;                  
		int Polynomial;                    

		DECLARE_MODEL_INTERFACE(CRC_Decoder);

		CRC_Decoder();
		~CRC_Decoder();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;
		bool Finalize() override;
		bool UpdateDynamicParameters() override;

//	private:
		int      m_InputFrmLen;   
		int      m_CRCLength;     
		uint32_t m_crcMask;       
		uint32_t m_polyNoMsb;     

		bool* m_msgFrame;   
		bool* m_msgLogical; 
		bool* m_crcRx;      
		bool* m_crcExp;     

		int  computeCRCLength(int poly) const;
		void computePolynomialMasks();
		int  boundaryCheck(char functionTag);

		void crcComputeRemainderBits(const bool* msgLogical, bool* crcBits /*len=r*/);
	};
}
