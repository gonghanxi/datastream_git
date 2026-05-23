#pragma once

#include "ModelBuilder.h"
#include <complex>
#include "SystemVueModels.h"

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

    class AddGuard : public DFModel
	{
	public:
		AddGuard();
		virtual ~AddGuard();

		DECLARE_MODEL_INTERFACE(AddGuard);

		DComplexCircularBuffer m_cbInput;   
		DComplexCircularBuffer m_cbOutput;  
		DoubleCircularBuffer   m_cbWindow;  

		int m_iIFFTSize;      
		int m_iPreGuard;      
		int m_iPostGuard;     
		int m_iIntersection;  

		virtual bool Setup() override;
		virtual bool Initialize() override;
		virtual bool Run() override;
		virtual bool Finalize() override;

	private:
		void ClearCplxBuffer();   

	protected:
		size_t m_iNout;
		size_t m_iNwin;
		size_t m_iNperiod;

		std::complex<double>* m_cplxBuffer;
	};

} 
