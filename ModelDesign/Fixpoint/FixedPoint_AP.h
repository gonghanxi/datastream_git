// Copyright  2011 - 2015 Keysight Technologies, Inc   
#pragma once

//----------------------------------------------------------------------------------------------
// This header provides methods to convert data between SystemVueModelBuilder FixedPoint datatype
// and Xilinx ap_fixed, ap_ufixed, ap_int, ap_uint. 
// Path to Xilinx ap_int.h header file must be included before including this header in the build system
//----------------------------------------------------------------------------------------------

#include "ap_int.h"
#include "SystemVue/FixedPoint.h"

namespace SystemVueModelBuilder
{
	extern void FixedPoint_To_RawAP(uint64_t* pResultVal, const FixedPointRep* fxpRep, const FixedPointParameters& fxpParams);
	extern void RawAP_To_FixedPoint(FixedPointRep* fxpRep, const FixedPointParameters& fxpParams, const uint64_t* c_pApVal, bool isNeg);

	/// Convert ap_(u)fixed/ap_(u)int to a FixedPoint value, 
	/// <param name="apVal"> The AP object which needs to be converted to FixedPoint</param>
	/// <param name="fxp"> The FixedPoint object, passed by reference, to store and return the converted value. </param>
	/// <remarks> Please note that this function will modify the precision of fxp if it is not the same as acVal </remarks>
	template <int WL>
	void  AP_To_FixedPoint(FixedPoint &fxp, const ap_int<WL> &apVal)
	{
		fxp.setParameters(WL, WL, FixedPointEnums::Sign::TWOS_COMPLEMENT);
		ConvertApToFxp(fxp, apVal);
	}

	template <int WL >
	void  AP_To_FixedPoint(FixedPoint &fxp, const ap_uint<WL> &apVal)
	{
		fxp.setParameters(WL, WL, FixedPointEnums::Sign::UNSIGNED);
		ConvertApToFxp(fxp, apVal);
	}

	template <int WL, int IWL, ap_q_mode _AP_Q, ap_o_mode _AP_O, int _AP_N>
	void  AP_To_FixedPoint(FixedPoint &fxp, const ap_fixed<WL, IWL, _AP_Q, _AP_O, _AP_N> &apVal)
	{
		fxp.setParameters(WL, IWL,
			FixedPointEnums::Sign::TWOS_COMPLEMENT, ConvertApQModeToFxpQMode(_AP_Q),
			ConvertApOModeToFxpOMode(_AP_O), _AP_N);
		ConvertApToFxp(fxp, apVal.V);		
	}
		
	template <int WL, int IWL, ap_q_mode _AP_Q, ap_o_mode _AP_O, int _AP_N>
	void  AP_To_FixedPoint(FixedPoint &fxp, const ap_ufixed<WL, IWL, _AP_Q, _AP_O, _AP_N> &apVal)
	{
		fxp.setParameters(WL, IWL,
			FixedPointEnums::Sign::UNSIGNED, ConvertApQModeToFxpQMode(_AP_Q),
			ConvertApOModeToFxpOMode(_AP_O), _AP_N);
		ConvertApToFxp(fxp, apVal.V);
	}

	// <= 64
	template<int W, bool S>
	void ConvertApToFxp(FixedPoint &fxp, const ap_private<W, S, true>& apPrivate)
	{
		uint64_t Val = apPrivate.get_VAL();
		SystemVueModelBuilder::RawAP_To_FixedPoint(fxp.getRep(), fxp.getParameters(), &Val, apPrivate.isNegative());
	}

	// > 64
	template<int W, bool S>
	void ConvertApToFxp(FixedPoint &fxp, const ap_private<W, S, false>& apPrivate)
	{
		const uint64_t* pVal = apPrivate.get_pVal();
		SystemVueModelBuilder::RawAP_To_FixedPoint(fxp.getRep(), fxp.getParameters(), pVal, apPrivate.isNegative());
	}
	

	/// Convert FixedPoint to ap_fixed  
	/// <param name="fxpVal"> The FixedPoint object which needs to be converted to ap_fixed. </param>
	/// <param name="apVal"> The AP object, passed by reference, to store and return the converted value</param>
	template <int WL>
	bool  FixedPoint_To_AP(ap_int<WL> &apVal, const FixedPoint &fxp)
	{
		if (VerifyFixedPointParameters(apVal, fxp))
		{
			ConvertFxpToAp(apVal, fxp);
			return true;
		}
		else
			return false;
	}

	template <int WL >
	bool  FixedPoint_To_AP(ap_uint<WL> &apVal, const FixedPoint &fxp)
	{
		if (VerifyFixedPointParameters(apVal, fxp))
		{
			ConvertFxpToAp(apVal, fxp);
			return true;
		}
		else
			return false;
	}

	template <int WL, int IWL, ap_q_mode _AP_Q, ap_o_mode _AP_O, int _AP_N>
	bool  FixedPoint_To_AP(ap_fixed<WL, IWL, _AP_Q, _AP_O, _AP_N> &apVal, const FixedPoint &fxp)
	{
		if (VerifyFixedPointParameters(apVal, fxp))
		{
			ConvertFxpToAp(apVal.V, fxp);
			return true;
		}
		else
			return false;
	}

	template <int WL, int IWL, ap_q_mode _AP_Q, ap_o_mode _AP_O, int _AP_N>
	bool  FixedPoint_To_AP(ap_ufixed<WL, IWL, _AP_Q, _AP_O, _AP_N> &apVal, const FixedPoint &fxp)
	{
		if (VerifyFixedPointParameters(apVal, fxp))
		{
			ConvertFxpToAp(apVal.V, fxp);
			return true;
		}
		else
			return false;
	}

	// <= 64
	template<int W, bool S>
	void ConvertFxpToAp(ap_private<W, S, true>& apPrivate, const FixedPoint &fxp)
	{
		uint64_t Val;
		SystemVueModelBuilder::FixedPoint_To_RawAP(&Val, const_cast<FixedPoint&>(fxp).getRep(), fxp.getParameters());
		apPrivate.set_VAL(Val);
	}

	// > 64
	template<int W, bool S>
	void ConvertFxpToAp(ap_private<W, S, false>& apPrivate, const FixedPoint &fxp)
	{
		uint64_t* pVal = apPrivate.get_pVal();
		SystemVueModelBuilder::FixedPoint_To_RawAP(pVal, const_cast<FixedPoint&>(fxp).getRep(), fxp.getParameters());
	}

	// 
	template <int WL>
	bool VerifyFixedPointParameters(const ap_int<WL> &val, const FixedPoint &fxp)
	{
		FixedPointParameters fxpParams = fxp.getParameters();

		// sign
		if (FixedPointEnums::TWOS_COMPLEMENT != fxpParams.sign())
		{
			return false;
		}

		// wl
		if (WL != fxpParams.wl())
		{
			return false;
		}

		// iwl - for ap_(u)int, the iwl must much the wl of the fxp 
		if (WL != fxpParams.iwl())
		{
			return false;
		}

		return true;
	}

	template <int WL>
	bool VerifyFixedPointParameters(const ap_uint<WL> &val, const FixedPoint &fxp)
	{
		FixedPointParameters fxpParams = fxp.getParameters();

		// sign
		if (FixedPointEnums::UNSIGNED != fxpParams.sign())
		{
			return false;
		}

		// wl
		if (WL != fxpParams.wl())
		{
			return false;
		}

		// iwl - for ap_(u)int, the iwl must much the wl of the fxp 
		if (WL != fxpParams.iwl())
		{
			return false;
		}

		return true;
	}

	template <int WL, int IWL, ap_q_mode _AP_Q, ap_o_mode _AP_O, int _AP_N>
	bool VerifyFixedPointParameters(const ap_fixed<WL, IWL, _AP_Q, _AP_O, _AP_N> &val, const FixedPoint &fxp)
	{
		FixedPointParameters fxpParams = fxp.getParameters();

		// sign
		if (FixedPointEnums::TWOS_COMPLEMENT != fxpParams.sign())
		{
			return false;
		}

		// wl
		if (WL != fxpParams.wl())
		{
			return false;
		}

		// iwl
		if (IWL != fxpParams.iwl())
		{
			return false;
		}

		if (ConvertApQModeToFxpQMode(_AP_Q) != fxpParams.q_mode())
		{
			return false;
		}

		if (ConvertApOModeToFxpOMode(_AP_O) != fxpParams.o_mode())
		{
			return false;
		}

		// saturation bits
		if (_AP_N != fxpParams.saturationBits())
		{
			return false;
		}

		return true;
	}

	template <int WL, int IWL, ap_q_mode _AP_Q, ap_o_mode _AP_O, int _AP_N>
	bool VerifyFixedPointParameters(const ap_ufixed<WL, IWL, _AP_Q, _AP_O, _AP_N> &val, const FixedPoint &fxp)
	{
		FixedPointParameters fxpParams = fxp.getParameters();

		// sign
		if (FixedPointEnums::UNSIGNED != fxpParams.sign())
		{
			return false;
		}

		// wl
		if (WL != fxpParams.wl())
		{
			return false;
		}

		// iwl
		if (IWL != fxpParams.iwl())
		{
			return false;
		}

		if (ConvertApQModeToFxpQMode(_AP_Q) != fxpParams.q_mode())
		{
			return false;
		}

		if (ConvertApOModeToFxpOMode(_AP_O) != fxpParams.o_mode())
		{
			return false;
		}

		// saturation bits
		if (_AP_N != fxpParams.saturationBits())
		{
			return false;
		}

		return true;
	}

	static FixedPointEnums::QuantizationMode ConvertApQModeToFxpQMode(ap_q_mode ap_q)
	{
		FixedPointEnums::QuantizationMode eQMode = FixedPointEnums::TRUNCATE;
		// extract QuantizationMode
		switch (ap_q)
		{
		case AP_TRN:
			eQMode = FixedPointEnums::TRUNCATE;
			break;
		case AP_RND:
			eQMode = FixedPointEnums::ROUND;
			break;
		case AP_TRN_ZERO:
			eQMode = FixedPointEnums::TRUNCATE_ZERO;
			break;
		case AP_RND_ZERO:
			eQMode = FixedPointEnums::ROUND_ZERO;
			break;
		case AP_RND_INF:
			eQMode = FixedPointEnums::ROUND_INFINITY;
			break;
		case AP_RND_MIN_INF:
			eQMode = FixedPointEnums::ROUND_MINUS_INFINITY;
			break;
		case AP_RND_CONV:
			eQMode = FixedPointEnums::ROUND_CONVERGENT;
			break;
		default:
			_ASSERT(false); // why are we here, it must be one of the above cases
		}

		return eQMode;
	}

	static FixedPointEnums::OverflowMode ConvertApOModeToFxpOMode(ap_o_mode ap_o)
	{
		FixedPointEnums::OverflowMode  eOMode = FixedPointEnums::WRAP;
		switch (ap_o)
		{
		case AP_WRAP:
			eOMode = FixedPointEnums::WRAP;
			break;
		case AP_SAT:
			eOMode = FixedPointEnums::SATURATE;
			break;
		case AP_SAT_ZERO:
			eOMode = FixedPointEnums::SATURATE_ZERO;
			break;
		case AP_SAT_SYM:
			eOMode = FixedPointEnums::SATURATE_SYMMETRICAL;
			break;
		case AP_WRAP_SM:
			eOMode = FixedPointEnums::WRAP_SIGN_MAGNITUDE;
			break;
		default:
			_ASSERT(false); // why are we here, it must be one of the above cases
		}

		return eOMode;
	}

}


