#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"
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

namespace SystemVueModelBuilder {

    class OSF : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(OSF);

		OSF();

		bool Setup() override;
		bool Run() override;

		bool UpdateDynamicParameters() override;

	protected:
		bool ValidateParameters();

		void OrderedInsert();

	public:
		double m_input;
		double m_output;

	public:
		int m_n;           
		int m_percentile;  

	protected:
		std::vector<double> m_window;

		std::vector<int> m_ranks;

		int m_current;

		int m_index;
	};

} 
