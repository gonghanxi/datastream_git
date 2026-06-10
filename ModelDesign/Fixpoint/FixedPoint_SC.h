// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

//----------------------------------------------------------------------------------------------
// This header provides methods to convert data between SystemVueModelBuilder FixedPoint datatype
// and SystemC sc_fixed, sc_ufixed, sc_int, sc_uint, sc_bigint, and sc_biguint. 
// Path to SystemC header file must be included before including this header in the build system
//----------------------------------------------------------------------------------------------

#include "systemc.h"
#include "sysc/datatypes/fx/sc_fixed.h"
#include "sysc/datatypes/fx/sc_ufixed.h"

#include "SystemVue/FixedPoint.h"


namespace SystemVueModelBuilder
{


	/// Converts SystemC quantization mode to SystemVueModelBuilder quantization mode
	///<param name="Q"> SystemC quantization mode sc_dt::sc_q_mode</param>
	///<returns> Correponding SystemVueModelBuilder quantization mode FixedPointEnums::QuantizationMode</returns>
	inline FixedPointEnums::QuantizationMode GetQuantizationMode(sc_dt::sc_q_mode Q)
	{
		FixedPointEnums::QuantizationMode eQMode;

		// extract QuantizationMode
		switch (Q)
		{
		case sc_dt::SC_TRN:
			eQMode = FixedPointEnums::TRUNCATE;
			break;
		case sc_dt::SC_RND:
			eQMode = FixedPointEnums::ROUND;
			break;
		case sc_dt::SC_TRN_ZERO:
			eQMode = FixedPointEnums::TRUNCATE_ZERO;
			break;
		case sc_dt::SC_RND_ZERO:
			eQMode = FixedPointEnums::ROUND_ZERO;
			break;
		case sc_dt::SC_RND_INF:
			eQMode = FixedPointEnums::ROUND_INFINITY;
			break;
		case sc_dt::SC_RND_MIN_INF:
			eQMode = FixedPointEnums::ROUND_MINUS_INFINITY;
			break;
		case sc_dt::SC_RND_CONV:
			eQMode = FixedPointEnums::ROUND_CONVERGENT;
			break;
		default:
			_ASSERT(false); // why are we here, it must be one of the above cases
		}

		return eQMode;
	}


	/// Converts SystemC Overflow mode to SystemVueModelBuilder overflow mode
	///<param name="O"> SystemC overflow mode sc_dt::sc_o_mode</param>
	///<returns> Correponding SystemVueModelBuilder overflow mode FixedPointEnums::OverflowMode</returns>
	inline FixedPointEnums::OverflowMode GetOverFlowMode(sc_dt::sc_o_mode O)
	{
		FixedPointEnums::OverflowMode  eOMode;

		// extract OverflowMode
		switch(O)
		{
		case sc_dt::SC_WRAP:
			eOMode = FixedPointEnums::WRAP;
			break;
		case sc_dt::SC_SAT:
			eOMode = FixedPointEnums::SATURATE;
			break;
		case sc_dt::SC_SAT_ZERO:
			eOMode = FixedPointEnums::SATURATE_ZERO;
			break;
		case sc_dt::SC_SAT_SYM:
			eOMode = FixedPointEnums::SATURATE_SYMMETRICAL;
			break;
		case sc_dt::SC_WRAP_SM:
			eOMode = FixedPointEnums::WRAP_SIGN_MAGNITUDE;
			break;
		default:
			_ASSERT(false); // why are we here, it must be one of the above cases
		}

		return eOMode;
	}

	/// Generate FixedPointParameters from sc_int
	/// <param name="val"> The templated ac_int object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_int object. </returns>
	template <int WL> 
	FixedPointParameters GetFixedPointParameters(const sc_dt::sc_int<WL> &val)
	{
		return FixedPointParameters(WL,WL,FixedPointEnums::TWOS_COMPLEMENT);
	}

	/// Generate FixedPointParameters from sc_bigint
	/// <param name="val"> The templated sc_bigint object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_bigint object. </returns>
	template <int WL> 
	FixedPointParameters GetFixedPointParameters(const sc_dt::sc_bigint<WL> &val)
	{
		return FixedPointParameters(WL,WL,FixedPointEnums::TWOS_COMPLEMENT);
	}

	/// Generate FixedPointParameters from sc_uint
	/// <param name="val"> The templated sc_uint object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_uint object. </returns>
	template <int WL> 
	FixedPointParameters GetFixedPointParameters(const sc_dt::sc_uint<WL> &val)
	{
		return FixedPointParameters(WL,WL,FixedPointEnums::UNSIGNED);
	}

	/// Generate FixedPointParameters from sc_biguint
	/// <param name="val"> The templated sc_biguint object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_biguint object. </returns>
	template <int WL> 
	FixedPointParameters GetFixedPointParameters(const sc_dt::sc_biguint<WL> &val)
	{
		return FixedPointParameters(WL,WL,FixedPointEnums::UNSIGNED);
	}

	/// Generate FixedPointParameters from sc_fixed
	/// <param name="val"> The templated sc_fixed object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_fixed object. </returns>
	template <int WL, int IWL, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int N> 
	FixedPointParameters GetFixedPointParameters(const sc_dt::sc_fixed<WL,IWL,Q,O,N> &val)
	{
		FixedPointEnums::QuantizationMode eQMode = GetQuantizationMode(Q);
		FixedPointEnums::OverflowMode  eOMode = GetOverFlowMode(O);
		return FixedPointParameters(WL,IWL,FixedPointEnums::TWOS_COMPLEMENT,eQMode,eOMode);
	}

	/// Generate FixedPointParameters from sc_ufixed
	/// <param name="val"> The templated sc_ufixed object from which FixedPointParameters needs to be extracted.</param>
	/// <returns>FixedPointParameters corresponding to input sc_ufixed object. </returns>
	template <int WL, int IWL, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int N> 
	FixedPointParameters GetFixedPointParameters(const sc_dt::sc_ufixed<WL,IWL,Q,O,N> &val)
	{
		FixedPointEnums::QuantizationMode eQMode = GetQuantizationMode(Q);
		FixedPointEnums::OverflowMode  eOMode = GetOverFlowMode(O);
		return FixedPointParameters(WL,IWL,FixedPointEnums::UNSIGNED,eQMode,eOMode);
	}

	/// Converts SystemC  data types to SystemVueModelBuilder FixedPoint data type
	/// <param name="scVal"> 
	/// SystemC data type object which needs to converted to FixedPoint. Currently supports
	/// sc_int, sc_uint, sc_bigint, sc_biguint, sc_fixed and sc_ufixed
	/// </param>
	/// <param name="fxpVal"> The FixedPoint object, passed by reference, to store and return the converted value. </param>
	/// <remarks> Please note that this function will modify the precision of fxpVal if it is not the same as scVal </remarks> 
	template<typename T> void SC_To_FixedPoint(const T &scVal, FixedPoint &fxpVal)
	{
		FixedPointParameters scParam = GetFixedPointParameters(scVal);
		FixedPointParameters fxpParam = fxpVal.getParameters();

		int iFwl = scParam.fwl();
		int iIwl = scParam.iwl();
		int j = 0;

		if(!(scParam == fxpParam))
		{
			fxpVal.setParameters(scParam);
		}

		for( int i = -iFwl; i < iIwl; ++ i,++j )
		{
			fxpVal[i] = scVal[j];
		}

	}


	/// Converts SystemVueModelBuilder FixedPoint data type to SystemC  data types 
	/// <param name="fxpVal"> The FixedPoint object, which needs to converted to SystemC data type. </param>
	/// <param name="scVal"> 
	/// SystemC data type object passed by reference, to store and return the converted value. Currently supports
	/// sc_int, sc_uint, sc_bigint, sc_biguint, sc_fixed and sc_ufixed
	/// </param>
	template <typename T> 
	void FixedPoint_To_SC(const FixedPoint &fxpVal, T &scVal)
	{	
		FixedPointParameters scParam = GetFixedPointParameters(scVal);
		FixedPointParameters fxpParam = fxpVal.getParameters();
		int iFwl = scParam.fwl();
		int iIwl = scParam.iwl();
		int j = 0;

		if(scParam == fxpParam)
		{
			// use the original fixed point
			for( int i = -iFwl; i < iIwl; ++i,++j )
			{
				scVal[j] = fxpVal[i];
			}
		}
		else
		{
			// create a new FixedPoint object with precision of scVal to convert values
			FixedPoint fxp;
			fxp.setParameters(scParam);
			fxp = fxpVal;
			for( int i = -iFwl; i < iIwl; ++i, ++j )
			{
				scVal[j] = fxp[i];
			}
		}
	}

}
