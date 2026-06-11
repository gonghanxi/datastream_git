// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

/*

Note this header is shipped with the modelbuidler.  No internal classes should be used 
in this header.  Also, updates should be made to minimize the need for customers to recompile 
their DLLs.

Increment the DFInterfaceVersion (defined in DFModel.h) to force a customer to recompile the DLL.

*/

#include <string>
#include "SystemVue/FixedPointValue.h"
#include "SystemVue/FixedPointParameters.h"
#include "SystemVue/FixedPointBitRef.h"
#include "SystemVue/CircularBuffer.h"
#include "SystemVue/DLL_Export/SystemC-FixedPoint.h"

// disable 'inconsistent dll linkage' warning
#pragma warning( disable:4273)

namespace SystemVueModelBuilder
{
	// Forward declaration
	class FixedPointRep;


    class SYSTEMC_FIXEDPOINT_API FixedPoint
	{
		// Proxy class for Bit referencing
		friend class FixedPointBitRef;
		// Arbitrary precision 
		friend class FixedPointValue;

	public:

		/// Signed or not
		bool isSigned() const;



		/// Default constructor
		FixedPoint();

		/// Constructor from int (for Matrix initialization compatibility)
		FixedPoint(int val);

		/// Constructor from double
		FixedPoint(double val);

		/// Copy constructor
		FixedPoint(const FixedPoint&);


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
		void setParameters(FixedPointEnums::Sign eSign, FixedPointEnums::QuantizationMode qm=FixedPointEnums::TRUNCATE, FixedPointEnums::OverflowMode om=FixedPointEnums::WRAP, int nb=0)
		{
			// Number of saturation bits must be greater than or equal to 0
			_ASSERT(nb >= 0);

			m_parameters.setParameters(eSign, qm, om, nb);
			cast();
		}


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
		void setParameters(int wl, int iwl, FixedPointEnums::Sign eSign, FixedPointEnums::QuantizationMode qm=FixedPointEnums::TRUNCATE, FixedPointEnums::OverflowMode om=FixedPointEnums::WRAP, int nb=0)
		{
			// Word Length must be greater than 0
			_ASSERT(wl > 0);

			// Number of saturation bits must be greater than or equal to 0
			_ASSERT(nb >= 0);

			m_parameters.setParameters(wl, iwl, eSign, qm,  om,  nb);
			cast();
		}


		void setParameters(const FixedPointParameters & fxParam)
		{
			// Word Length must be greater than 0
			_ASSERT(fxParam.wl() > 0);

			// Number of saturation bits must be greater than or equal to 0
			_ASSERT(fxParam.saturationBits() >= 0);

			m_parameters = fxParam;
			cast();
		}


		/// Destructor
        ~FixedPoint();



		/// Get FixedPointValue
		operator FixedPointValue () const;


		/// --- Unary operators ----------------------------------------------

		const FixedPointValue operator - () const;
		const FixedPointValue operator + () const;


		/// --- Binary operators FixedPoint vs FixedPoint -----------------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, const FixedPoint& );


		/// --- Binary operators FixedPoint vs const FixedPointValue& ------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, const FixedPoint& );



		/// --- Binary operators FixedPoint vs int -----------------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( int, const FixedPoint& );


		/// --- Binary operators FixedPoint vs unsigned int ----------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( unsigned int, const FixedPoint& );



		/// --- Binary operators FixedPoint vs long -----------------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( long, const FixedPoint& );


		/// --- Binary operators FixedPoint vs unsigned long --------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( unsigned long, const FixedPoint& );


		/// --- Binary operators FixedPoint vs double ---------------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( double, const FixedPoint& );

		/// ---- Binary Shift Operators ----------------------------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator << ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator >> ( const FixedPoint&, int );


		/// --- Relational (including equality) operators FixedPoint vs FixedPoint -----------

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, const FixedPoint& );


		/// ---- Relational (including equality) operators FixedPoint vs const FixedPointValue& ---

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, const FixedPoint& );

		/// --- Relational (including equality) operators FixedPoint vs int ------------

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, int );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( int, const FixedPoint& );

		/// --- Relational (including equality) operators FixedPoint vs unsigned int ----

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( unsigned int, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, unsigned int );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( unsigned int, const FixedPoint& );

		/// --- Relational (including equality) operators FixedPoint vs long -----------

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, long );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( long, const FixedPoint& );


		/// --- Relational (including equality) operators FixedPoint vs unsigned long ---

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( unsigned long, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, unsigned long );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( unsigned long, const FixedPoint& );

		/// --- Relational (including equality) operators FixedPoint vs double -----------

		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( double, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPoint&, double );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( double, const FixedPoint& );




		/// ---- Unary bitwise operators -------------------

		const FixedPoint operator ~ () const;


		/// ---- Binary bitwise operators ------------------

		friend SYSTEMC_FIXEDPOINT_API const FixedPoint operator & ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPoint operator | ( const FixedPoint&, const FixedPoint& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPoint operator ^ ( const FixedPoint&, const FixedPoint& );



		/// ------ Assignment operators for int ------------

		FixedPoint& operator =( int a );
		FixedPoint& operator *=( int a );
		FixedPoint& operator /=( int a );
		FixedPoint& operator +=( int a );
		FixedPoint& operator -=( int a );

		/// Assignment operators for unsigned int -----------

		FixedPoint& operator =( unsigned int a );
		FixedPoint& operator *=( unsigned int a );
		FixedPoint& operator /=( unsigned int a );
		FixedPoint& operator +=( unsigned int a );
		FixedPoint& operator -=( unsigned int a );

		/// ---- Assignment operators for long --------------

		FixedPoint& operator =( long a );
		FixedPoint& operator *=( long a );
		FixedPoint& operator /=( long a );
		FixedPoint& operator +=( long a );
		FixedPoint& operator -=( long a );

		/// --- Assignment operators for unsigned long ------

		FixedPoint& operator =( unsigned long a );
		FixedPoint& operator *=( unsigned long a );
		FixedPoint& operator /=( unsigned long a );
		FixedPoint& operator +=( unsigned long a );
		FixedPoint& operator -=( unsigned long a );

		/// --- Assignment operators for double -------------

		FixedPoint& operator =( double a );
		FixedPoint& operator *=( double a );
		FixedPoint& operator /=( double a );
		FixedPoint& operator +=( double a );
		FixedPoint& operator -=( double a );

		/// --- Assignment operators for const FixedPointValue& ------

		FixedPoint& operator =( const FixedPointValue& a );
		FixedPoint& operator *=( const FixedPointValue& a );
		FixedPoint& operator /=( const FixedPointValue& a );
		FixedPoint& operator +=( const FixedPointValue& a );
		FixedPoint& operator -=( const FixedPointValue& a );

		/// --- Assignment operators for const FixedPoint& --------

		FixedPoint& operator =( const FixedPoint& a );
		FixedPoint& operator *=( const FixedPoint& a );
		FixedPoint& operator /=( const FixedPoint& a );
		FixedPoint& operator +=( const FixedPoint& a );
		FixedPoint& operator -=( const FixedPoint& a );

		FixedPoint& operator <<=( int a );
		FixedPoint& operator >>=( int a );



		/// ----- Auto-increment and auto-decrement -------

		const FixedPointValue operator ++ ( int n );
		const FixedPointValue operator -- ( int n );

		FixedPoint& operator ++ ();
		FixedPoint& operator -- ();


		/// ------ Bit selection methods/operators -------

		const FixedPointBitRef operator [] ( int i ) const;
		FixedPointBitRef       operator [] ( int i );

		/// Returns i^th bit reference. The value of i can be a -ve value
		/// representation fractional part. 
		const FixedPointBitRef bit( int i ) const;

		/// Returns i^th bit reference. The value of i can be a -ve value
		/// representation fractional part
		/// Any operation performed on the return value will be performed on the
		//	actual bit in the corresponding FixedPoint point
		FixedPointBitRef      bit( int i );


		/// -------- Explicit conversion methods ---------

		/// Explicit conversion to short
		short          to_short() const;

		/// Explicit conversion to unsigned short
		unsigned short to_ushort() const;

		/// Explicit conversion to int
		int            to_int() const;

		/// Explicit conversion to unsigned int
		unsigned int   to_uint() const;

		/// Explicit conversion to long
		long           to_long() const;

		/// Explicit conversion to unsigned long
		unsigned long  to_ulong() const;

		/// Explicit conversion to float
		float          to_float() const;

		/// Explicit conversion to double
		double         to_double() const;


		/// Explicit conversion to character string in decimal format
		const std::string to_dec() const;

		/// Explicit conversion to character string in binary format
		const std::string to_bin() const;

		/// Explicit conversion to character string in oct format
		const std::string to_oct() const;

		/// Explicit conversion to character string in hex format
		const std::string to_hex() const;


		// ------ Query Methods ----------------

		/// Returns true if negative
		bool is_neg() const;

		/// Returns true if Zero
		bool is_zero() const;

		// internal use only;
		bool is_normal() const;

		/// Returns true if quantization flag is set
		bool quantization_flag() const;

		/// Returns true if overflow flag is set
		bool overflow_flag() const;



		/// Returns word length
		int       wl() const;

		/// Returns integer word length
		int       iwl() const;

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
		FixedPointEnums::QuantizationMode       q_mode() const;


		/// Returns Overflow mode. The return mode can have one of the following values
		/// <list type="bullet">
		///			<item> <description>FixedPointEnums::SATURATE: Saturation </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_ZERO: Saturation to Zero </description> </item>
		///			<item> <description>FixedPointEnums::SATURATE_SYMMETRICAL: Symmetrical saturation </description>  </item>	
		///			<item> <description>FixedPointEnums::WRAP: Wrap-around </description> </item>
		///			<item> <description>FixedPointEnums::WRAP_SIGN_MAGNITUDE: Sign magnitude wrap-around </description> </item>
		///		</list>
		FixedPointEnums::OverflowMode       o_mode() const;

		// Returns sign, TWOS_COMPLEMENT or UNSIGNED
		FixedPointEnums::Sign				sign() const;

		/// Returns number of saturation bits used for FixedPointEnums::WRAP and FixedPointEnums::WRAP_SIGN_MAGNITUDE Overflow modes.
		int       saturationBits() const;

		/// Returns fixed point parameters
		const FixedPointParameters &  getParameters() const;


		// ------ End of Query Methods ----------------


		/// Initialize FixedPoint, inheriting fixpt-precision of the reference.  
		// Internal use only
		bool Zero(FixedPoint* pReference);

		// Internal use only
		inline FixedPointRep*  getRep()
		{
			return m_rep;
		}

	private:

		FixedPointRep*             m_rep;
		FixedPointParameters       m_parameters;
		bool                       m_quantizationFlag;
		bool                       m_overflowFlag;

		// Parameter cast to set/reset overflow/underflow flags
		void cast();
		// internal use only;
		bool get_bit( int ) const;
		bool set_bit( int, bool );

	};


#pragma warning(disable: 4275)

	/// Circular buffer for fix data type using template specialization
	template <> class CircularBuffer<FixedPoint> : public CircularBufferE<FixedPoint>
	{
	public:
		CircularBuffer<FixedPoint>():
		  CircularBufferE<FixedPoint>(), 
			  m_parameters(0,0,FixedPointEnums::TWOS_COMPLEMENT)	
		  {
		  }

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
		  void SetParameters(int wl, int iwl, FixedPointEnums::Sign eSign, FixedPointEnums::QuantizationMode qm=FixedPointEnums::TRUNCATE, FixedPointEnums::OverflowMode om=FixedPointEnums::WRAP, int nb=0)
		  {
			  // Word Length must be greater than 0
			  _ASSERT(wl > 0);

			  // Number of saturation bits must be greater than or equal to 0
			  _ASSERT(nb >= 0);

			  m_parameters.setParameters(wl, iwl, eSign, qm,  om,  nb);
		  }

		  /// Sets FixedPointed  parameters
		  /// <param name="fxParam"> Input FixedPointParameters.</param>
		  void SetParameters(const FixedPointParameters & fxParam)
		  {
			  // Word Length must be greater than 0
			  _ASSERT(fxParam.wl() > 0);

			  // Number of saturation bits must be greater than or equal to 0
			  _ASSERT(fxParam.saturationBits() >= 0);
			  m_parameters = fxParam;
		  }

		  /// Returns and object of FixedPointParameters
		  const FixedPointParameters&  GetParameters() const
		  {
			  return m_parameters;
		  }

		  /// returns true if parameters are explicitly set previously, false otherwise 
		  bool AreParametersValid() const
		  {
			  // this means parameters are set for this circular buffer explicitly.
			  return (m_parameters.wl() > 0);
		  }

	private:
		/// The precision that was propagated
		FixedPointParameters m_parameters;
	};


	/// Typedef for fixed point circular buffer
	typedef CircularBuffer<FixedPoint>  FixedPointCircularBuffer;

	/// Circular buffer bus for FixedPoint data type
	typedef CircularBufferBusT<FixedPointCircularBuffer> FixedPointCircularBufferBus;

#pragma warning(default: 4275)
}
