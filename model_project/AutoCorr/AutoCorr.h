#pragma once

#include "ModelBuilder.h"
#include "SystemVueModels.h"
#include <vector>

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
    class AutoCorr : public DFModel
	{
	public:
		AutoCorr();

		CircularBuffer<double> input;
		CircularBuffer<double> output;

		enum CorrelationType
		{
			NonCircular = 0,
			Circular = 1
		};
		CorrelationType m_CorrelationType;   

		int CorrelationLength;               

		int StartLag;                       
		int StopLag;                         

		enum Normalization
		{
			None = 0,
			UnBiased = 1,
			Biased = 2
		};
		Normalization m_Normalization;       

		bool Setup();                
		bool Initialize();           
		bool Run();                  
		bool Finalize();             
		bool UpdateDynamicParameters(); 

		DECLARE_MODEL_INTERFACE(AutoCorr);

	private:
		double nonCircularAutoCorrelation(int lag);

		double circularAutoCorrelation(int lag);

		std::vector<double> m_samples;

		int m_numLags;
	};
}
