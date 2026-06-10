// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

//----------------------------------------------------------------------------------------------
// This header provides methods to convert data between SystemVue FixedPoint datatype
// and Mentor Graphics (R) Algorithmic C datatypes. 
// Path to Algorithmic C datatype header file "ac_fixed.h" must be included before including this 
// header in the build system.
//----------------------------------------------------------------------------------------------


#include "SystemVue/ac_types/ac_fixed.h"
#include "SystemVue/FixedPoint.h"


namespace SystemVueModelBuilder
{

	/// Generate FixedPointParameters from ac_fixed
	/// <param name="val"> The templated ac_fixed object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_fixed object. </returns>
	template <int WL, int IWL,bool S, ac_q_mode Q, ac_o_mode O> 
	FixedPointParameters GetFixedPointParameters(const ac_fixed<WL,IWL,S,Q,O> &val)
	{
		FixedPointEnums::QuantizationMode eQMode;
		FixedPointEnums::OverflowMode  eOMode;

		// Extract Sign
		FixedPointEnums::Sign eSign = S ? FixedPointEnums::TWOS_COMPLEMENT : FixedPointEnums::UNSIGNED;
		
		// extract QuantizationMode
		switch (Q)
		{
		case AC_TRN:
			eQMode = FixedPointEnums::TRUNCATE;
			break;
		case AC_RND:
			eQMode = FixedPointEnums::ROUND;
			break;
		case AC_TRN_ZERO:
			eQMode = FixedPointEnums::TRUNCATE_ZERO;
			break;
		case AC_RND_ZERO:
			eQMode = FixedPointEnums::ROUND_ZERO;
			break;
		case AC_RND_INF:
			eQMode = FixedPointEnums::ROUND_INFINITY;
			break;
		case AC_RND_MIN_INF:
			eQMode = FixedPointEnums::ROUND_MINUS_INFINITY;
			break;
		case AC_RND_CONV:
			eQMode = FixedPointEnums::ROUND_CONVERGENT;
			break;
		default:
			_ASSERT(false); // why are we here, it must be one of the above cases
		}

		// extract OverflowMode
		switch(O)
		{
		case AC_WRAP:
			eOMode = FixedPointEnums::WRAP;
			break;
		case AC_SAT:
			eOMode = FixedPointEnums::SATURATE;
			break;
		case AC_SAT_ZERO:
			eOMode = FixedPointEnums::SATURATE_ZERO;
			break;
		case AC_SAT_SYM:
			eOMode = FixedPointEnums::SATURATE_SYMMETRICAL;
			break;
		default:
			_ASSERT(false); // why are we here, it must be one of the above cases
		}

		return FixedPointParameters(WL,IWL,eSign,eQMode,eOMode);
	}

	/// Generate FixedPointParameters from ac_int
	/// <param name="val"> The templated ac_int object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_int object. </returns>
	template <int WL, bool S> 
	FixedPointParameters GetFixedPointParameters(const ac_int<WL,S> &val)
	{
		FixedPointEnums::Sign eSign = S ? FixedPointEnums::TWOS_COMPLEMENT : FixedPointEnums::UNSIGNED;
		return FixedPointParameters(WL,WL,eSign);
	}



	/// Convert ac_fixed to a FixedPoint value, 
	/// <param name="acVal"> The AC object which needs to be converted to FixedPoint. Currently supports only ac_fixed and ac_int </param>
	/// <param name="fxp"> The FixedPoint object, passed by reference, to store and return the converted value. </param>
	/// <remarks> Please note that this function will modify the precision of fxp if it is not the same as acVal </remarks>
	template <typename T> 
	void  AC_To_FixedPoint(T &acVal, FixedPoint &fxp)
	{
		int j = 0;
		FixedPointParameters acParam = GetFixedPointParameters(acVal);
		FixedPointParameters fxpParam = fxp.getParameters();
		int iFwl = acParam.fwl();
		int iIwl = acParam.iwl();

		if(!(acParam == fxpParam))
		{
			fxp.setParameters(acParam);
		}
		
		for( int i = -iFwl; i < iIwl; ++ i , ++j)
		{
			fxp[i] = acVal[j];
		}
	}


	

	/// Convert FixedPoint to ac_fixed  
	/// <param name="fxpVal"> The FixedPoint object which needs to be converted to ac_fixed. </param>
	/// <param name="acVal"> The AC object, passed by reference, to store and return the converted value. Currently supports only ac_fixed and ac_int </param>
	template <typename T> 
	void FixedPoint_To_AC(const FixedPoint &fxpVal,T &acVal)
	{	
		int j = 0;
		FixedPointParameters acParam = GetFixedPointParameters(acVal);
		FixedPointParameters fxpParam = fxpVal.getParameters();
		int iFwl = acParam.fwl();
		int iIwl = acParam.iwl();

		if(acParam == fxpParam)
		{
			// use the original fixed point
			for( int i = -iFwl; i < iIwl; ++ i , ++j)
			{
				acVal[j] = fxpVal[i];
			}
		}
		else
		{
			// create a new FixedPoint object with precision of acVal to convert values
			FixedPoint fxp;
			fxp.setParameters(acParam);
			fxp = fxpVal;
			for( int i = -iFwl; i < iIwl; ++ i , ++j)
			{
				acVal[j] = fxp[i];
			}
		}
	}

}


