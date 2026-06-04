#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{

	class SYSTEMVUEMODELBUILDER_API PattMatch : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(PattMatch);

		PattMatch();

		bool Setup() override;
		bool Run() override;

		bool ValidateParameters();

	public:
		CircularBuffer<double> m_templ;   
		CircularBuffer<double> m_window;  

		CircularBuffer<double> m_values;  
		int                    m_index;   

	public:
		int m_tempSize;   
		int m_winSize;    

		int m_n;
	};

} 
