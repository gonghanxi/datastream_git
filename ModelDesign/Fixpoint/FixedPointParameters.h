// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

#include "SystemVue/DLL_Export/SystemC-FixedPoint.h"
#include "SystemVue/FixedPointEnums.h"

namespace SystemVueModelBuilder
{

	// classes defined in this module
	class FixedPointParameters;


	// ----------------------------------------------------------------------------
	//  CLASS : FixedPointParameters
	//
	// ----------------------------------------------------------------------------

	class SYSTEMC_FIXEDPOINT_API FixedPointParameters
	{

	public:

		// Default Constructor
		FixedPointParameters();

		// Copy Constructor
		FixedPointParameters(const FixedPointParameters &rhs);

		//Overloaded constructor
		FixedPointParameters(int wl, int iwl, FixedPointEnums::Sign eSign, FixedPointEnums::QuantizationMode qm=FixedPointEnums::TRUNCATE, FixedPointEnums::OverflowMode om=FixedPointEnums::WRAP, int nb=0);


		/// Sets FixedPointed  parameters
		/// <param name="wl"> Word length in bits, wl must be greater than zero.</param>
		/// <param name="iwl"> Integer word length in bits, iwl can be negative or positive.</param>
		/// <param name="eSign"> FixedPointEnums::TWOS_COMPLEMENT  OR FixedPointEnums::UNSIGNED .</param>
		/// <param name="qm"> Specifies the quantization mode, possible values are, Note that "FixedPointEnums" is a nested namespace inside SystemVueModelBuilder namespace (SystemVueModelBuilder::FixedPointEnums)
		///		<list type="bullet">
		///			<item> <description>FixedPointEnums::ROUND - Rounding to Plus infinity </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_ZERO - Rounding to Zero </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_MINUS_INFINITY - Rounding to Minus infinity </description>  </item>	
		///			<item> <description>FixedPointEnums::ROUND_INFINITY - Rounding to infinity </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_CONVERGENT - Convergent rounding </description> </item>
		///			<item> <description>FixedPointEnums::TRUNCATE -  Truncation </description> </item>
		///			<item> <description>FixedPointEnums::TRUNCATE_ZERO -  Truncation to zero </description> </item>
		///		</list>
		/// </param>
		/// <param name="om"> Specifies the overflow mode, possible values are. Note that "FixedPointEnums" is a nested namespace inside SystemVueModelBuilder namespace (SystemVueModelBuilder::FixedPointEnums)
		///		<list type="bullet">
		///			<item> <description>FixedPointEnums::SATURATE - Saturation </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_ZERO - Saturation to Zero </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_SYMMETRICAL - Symmetrical saturation </description>  </item>	
		///			<item> <description>FixedPointEnums::WRAP - Wrap-around </description> </item>
		///			<item> <description>FixedPointEnums::WRAP_SIGN_MAGNITUDE - Sign magnitude wrap-around </description> </item>
		///		</list>
		/// </param>
		/// <param name="nb"> This parameter is used to provide number of saturation bits for FixedPointEnums::WRAP and FixedPointEnums::WRAP_SIGN_MAGNITUDE Overflow modes.</param>
		void setParameters(int wl, int iwl, FixedPointEnums::Sign eSign, FixedPointEnums::QuantizationMode qm=FixedPointEnums::TRUNCATE, FixedPointEnums::OverflowMode om=FixedPointEnums::WRAP, int nb=0);

		/// Sets FixedPoint  parameters
		/// <param name="eSign"> FixedPointEnums::TWOS_COMPLEMENT  OR FixedPointEnums::UNSIGNED .</param>
		/// <param name="qm"> Specifies the quantization mode, possible values are. Note that "FixedPointEnums" is a nested namespace inside SystemVueModelBuilder namespace (SystemVueModelBuilder::FixedPointEnums)
		///		<list type="bullet">
		///			<item> <description>FixedPointEnums::ROUND - Rounding to Plus infinity </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_ZERO - Rounding to Zero </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_MINUS_INFINITY - Rounding to Minus infinity </description>  </item>	
		///			<item> <description>FixedPointEnums::ROUND_INFINITY - Rounding to infinity </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_CONVERGENT - Convergent rounding </description> </item>
		///			<item> <description>FixedPointEnums::TRUNCATE -  Truncation </description> </item>
		///			<item> <description>FixedPointEnums::TRUNCATE_ZERO -  Truncation to zero </description> </item>
		///		</list>
		/// </param>
		/// <param name="om"> Specifies the overflow mode, possible values are. Note that "FixedPointEnums" is a nested namespace inside SystemVueModelBuilder namespace (SystemVueModelBuilder::FixedPointEnums)
		///		<list type="bullet">
		///			<item> <description>FixedPointEnums::SATURATE - Saturation </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_ZERO - Saturation to Zero </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_SYMMETRICAL - Symmetrical saturation </description>  </item>	
		///			<item> <description>FixedPointEnums::WRAP - Wrap-around </description> </item>
		///			<item> <description>FixedPointEnums::WRAP_SIGN_MAGNITUDE - Sign magnitude wrap-around </description> </item>
		///		</list>
		/// </param>
		/// <param name="nb"> This parameter is used to provide number of saturation bits for FixedPointEnums::WRAP and FixedPointEnums::WRAP_SIGN_MAGNITUDE Overflow modes.</param>
		void setParameters(FixedPointEnums::Sign eSign, FixedPointEnums::QuantizationMode qm=FixedPointEnums::TRUNCATE, FixedPointEnums::OverflowMode om=FixedPointEnums::WRAP, int nb=0);



		// query functions

		// Returns sign, TWOS_COMPLEMENT or UNSIGNED
		FixedPointEnums::Sign sign() const;

		/// Returns word length
		int wl() const;

		/// Returns integer word length
		int iwl() const;

		/// Returns fractional word length
		int fwl() const;

		/// Returns quantization mode. The return mode can have one of the following values
		/// <list type="bullet">
		///			<item> <description>FixedPointEnums::ROUND: Rounding to Plus infinity </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_ZERO: Rounding to Zero </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_MINUS_INFINITY: Rounding to Minus infinity </description>  </item>	
		///			<item> <description>FixedPointEnums::ROUND_INFINITY: Rounding to infinity </description> </item>
		///			<item> <description>FixedPointEnums::ROUND_CONVERGENT: Convergent rounding </description> </item>
		///			<item> <description>FixedPointEnums::TRUNCATE:  Truncation </description> </item>
		///			<item> <description>FixedPointEnums::TRUNCATE_ZERO:  Truncation to zero </description> </item>
		///		</list>
		FixedPointEnums::QuantizationMode q_mode() const;

		/// Returns Overflow mode. The return mode can have one of the following values
		/// <list type="bullet">
		///			<item> <description>FixedPointEnums::SATURATE: Saturation </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_ZERO: Saturation to Zero </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_SYMMETRICAL: Symmetrical saturation </description>  </item>	
		///			<item> <description>FixedPointEnums::WRAP: Wrap-around </description> </item>
		///			<item> <description>FixedPointEnums::WRAP_SIGN_MAGNITUDE: Sign magnitude wrap-around </description> </item>
		///		</list>
		FixedPointEnums::OverflowMode o_mode() const;

		/// Returns number of saturation bits used for FixedPointEnums::WRAP and FixedPointEnums::WRAP_SIGN_MAGNITUDE Overflow modes.
		int saturationBits() const;

		// equality test
		bool operator == ( FixedPointParameters& cfpX );


	private:

		FixedPointEnums::Sign	m_sign;
		int						m_wl;
		int						m_iwl;
		int						m_saturationBits;
		FixedPointEnums::QuantizationMode	m_quantizationMode;
		FixedPointEnums::OverflowMode		m_overflowMode;
		

	};




} // namespace SystemVueModelBuilder
