// Copyright 2011 - 2014 Keysight Technologies, Inc   

#pragma once

#include "SystemVue/eresult.h"
#include "SystemVue/FixedPoint.h"
#include "SystemVue/SystemVue.h"

namespace SystemVueModelBuilder
{
	class SYSTEMVUEMODELBUILDER_API DFFixedPointInterface
	{
	public:
		virtual ERESULT SetOutputFixedPointParameters()
		{		
			return E_NOTIMPL_;
		}
	};
}
