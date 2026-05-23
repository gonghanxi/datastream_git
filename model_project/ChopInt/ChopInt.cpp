#include "ChopInt.h"

#include <algorithm>

namespace SystemVueModelBuilder
{

#ifndef SV_CODE_GEN
	DEFINE_MODEL_INTERFACE(ChopInt)
	{
		SET_MODEL_DESCRIPTION("Data Block Chopper");
		SET_MODEL_SYMBOL("SYM_Chop");
		SET_MODEL_CATEGORY("Signal Processing");

		{
			DFPort p = ADD_MODEL_INPUT(input);
			p.SetDescription("input signal");
		}
		{
			DFPort p = ADD_MODEL_OUTPUT(output);
			p.SetDescription("output signal");
		}

		{
			DFParam p = ADD_MODEL_PARAM(nRead);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("128");
			p.SetDescription("Number of data items read");
		}

		{
			DFParam p = ADD_MODEL_PARAM(nWrite);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("64");
			p.SetDescription("Number of data items written");
		}

		{
			DFParam p = ADD_MODEL_PARAM(Offset);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("0");
			p.SetDescription("Start of output block relative to start of input block");
		}

		{
			DFParam p = ADD_MODEL_ENUM_PARAM(UsePastInputs, QueryEnum);
			p.SetUnit(Units::NONE);
			p.AddEnumeration("NO", QUERY_NO);
			p.AddEnumeration("YES", QUERY_YES);
			p.SetDefaultValue("YES");
			p.SetDescription("Use previously read inputs");
		}

		return true;
	}
#endif 

	ChopInt::ChopInt()
		: input()
		, output()
		, nRead(128)
		, nWrite(64)
		, Offset(0)
		, UsePastInputs(QUERY_YES)
		, iReadFrom(0)
		, iReadNum(0)
		, iReadBufSize(0)
		, iWriteTo(0)
		, iWriteNum(0)
		, iWriteBufSize(0)
		, iZeroPadFrom(0)
		, iZeroPadNum(0)
	{
	}

	bool ChopInt::Setup()
	{
		if ((nRead > 0) && (nWrite > 0))
		{
			iReadBufSize = static_cast<std::size_t>(nRead);

			if ((UsePastInputs == QUERY_YES) && (Offset > 0))
			{
				iReadBufSize += static_cast<std::size_t>(Offset);
			}

			input.SetRate(static_cast<unsigned int>(nRead));
			input.SetHistoryDepth(iReadBufSize);

			iWriteBufSize = static_cast<std::size_t>(nWrite);
			output.SetRate(static_cast<unsigned int>(nWrite));
		}

		return true;
	}

	bool ChopInt::Initialize()
	{
		bool bSuccess = true;

		if (nRead <= 0)
		{
			POST_ERROR("nRead must be > 0");
			bSuccess = false;
		}

		if (nWrite <= 0)
		{
			POST_ERROR("nWrite must be > 0");
			bSuccess = false;
		}

		if (bSuccess)
		{
			ComputeRange();
		}

		return bSuccess;
	}

	bool ChopInt::Run()
	{
		if (iWriteTo > 0)
		{
			output.Zero(0, iWriteTo, input.GetPointer(0));
		}

		if (iWriteNum > 0)
		{
			input.Copy(iReadFrom, &output, iWriteTo, iWriteNum);
		}

		if (iZeroPadNum > 0)
		{
			output.Zero(iZeroPadFrom, iZeroPadNum, input.GetPointer(0));
		}

		return true;
	}

	void ChopInt::ComputeRange()
	{
		if ((UsePastInputs == QUERY_YES) && (Offset > 0))
		{
			iReadFrom = 0;
			iWriteTo = 0;
		}
		else if (Offset > 0)   
		{
			iReadFrom = 0;
			iWriteTo = static_cast<std::size_t>(Offset);
		}
		else                   
		{
			iReadFrom = static_cast<std::size_t>(-Offset);
			iWriteTo = 0;
		}

		if (iWriteTo >= iWriteBufSize)
		{
			iWriteNum = 0;
			iWriteTo = iWriteBufSize; 
			iZeroPadNum = 0;
		}
		else
		{
			std::size_t availableFromInput =
				(iReadFrom < iReadBufSize) ? (iReadBufSize - iReadFrom) : 0;

			iWriteNum = (std::min)(iWriteBufSize - iWriteTo, availableFromInput);

			iZeroPadFrom = iWriteNum + iWriteTo;

			if (iZeroPadFrom < iWriteBufSize)
			{
				iZeroPadNum = iWriteBufSize - iZeroPadFrom;
			}
			else
			{
				iZeroPadNum = 0;
			}
		}
	}

} 

