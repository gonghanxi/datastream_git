#include "ChopVarOffset.h"

#include <algorithm>

namespace SystemVueModelBuilder
{

#ifndef SV_CODE_GEN
	DEFINE_MODEL_INTERFACE(ChopVarOffset)
	{
		SET_MODEL_DESCRIPTION("Data Block chopper with offset Control");
		SET_MODEL_SYMBOL("SYM_ChopVarOffset");
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
			DFPort p = ADD_MODEL_INPUT(offsetCntrl);
			p.SetDescription("offset control signal");
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

		return true;
	}
#endif  

	ChopVarOffset::ChopVarOffset()
		: input()
		, offsetCntrl()
		, output()
		, nRead(128)
		, nWrite(64)
		, Offset(0)
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

	bool ChopVarOffset::Setup()
	{
		if ((nRead > 0) && (nWrite > 0))
		{
			iReadBufSize = static_cast<std::size_t>(nRead);

			input.SetRate(static_cast<unsigned int>(nRead));
			input.SetHistoryDepth(iReadBufSize);

			iWriteBufSize = static_cast<std::size_t>(nWrite);
			output.SetRate(static_cast<unsigned int>(nWrite));

			offsetCntrl.SetRate(1U);
		}

		return true;
	}

	bool ChopVarOffset::Initialize()
	{
		bool ok = true;

		if (nRead <= 0)
		{
			POST_ERROR("nRead must be > 0");
			ok = false;
		}

		if (nWrite <= 0)
		{
			POST_ERROR("nWrite must be > 0");
			ok = false;
		}

		if (ok)
		{
			Offset = 0;
			ComputeRange();
		}

		return ok;
	}

	bool ChopVarOffset::Run()
	{
		Offset = offsetCntrl[0];

		ComputeRange();

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

	void ChopVarOffset::ComputeRange()
	{
		if (Offset > 0)
		{
			iReadFrom = 0;
			iWriteTo = static_cast<std::size_t>(Offset);
		}
		else 
		{
			long long negK = -(static_cast<long long>(Offset)); // ·ÀÖ¹Ö±½Ó -Offset Òç³ö
			if (negK < 0) negK = 0; 
			iReadFrom = static_cast<std::size_t>(negK);
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

			iZeroPadFrom = iWriteTo + iWriteNum;

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
