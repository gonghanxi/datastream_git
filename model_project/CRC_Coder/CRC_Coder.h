#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVueModels.h"

#include <cstdint>

namespace SystemVueModelBuilder
{
    class CRC_Coder : public DFModel
	{
	public:
		CRC_Coder();
		~CRC_Coder();

		bool Setup() override;
		bool Initialize() override;

		bool Run() override;

		bool Finalize() override;

		bool UpdateDynamicParameters();

		DECLARE_MODEL_INTERFACE(CRC_Coder);

		BoolCircularBuffer In;
		BoolCircularBuffer Out;

		enum ParityPositionEnum { Tail = 0, Head = 1 };
		ParityPositionEnum ParityPosition;   

		enum YesNoEnum { NO = 0, YES = 1 };
		YesNoEnum ReverseData;        
		YesNoEnum ReverseParity;      
		YesNoEnum ComplementParity;   

		int MessageLength;  
		int InitialState;   
		int Polynomial;     

//	private:
		int      m_OutFrmLen;       
		int      m_CRCLength;       
		uint32_t m_crcMask;         
		uint32_t m_polyNoMsb;       

		bool* m_frameP;             
		bool* m_CRC_P;              

//	private:
		// helpers
		int  boundaryCheck(char functionTag);
		int  computeCRCLength(int poly) const;
		void computePolynomialMasks();
		void crcEncodeOneFrame(const bool* dataBits, bool* crcBits);
	};
}
