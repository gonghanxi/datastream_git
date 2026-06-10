// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

#include "SystemVue/FixedPointEnums.h"
#include "SystemVue/DLL_Export/SystemC-FixedPoint.h"

#include <string>

// disable 'inconsistent dll linkage' warning
#pragma warning( disable:4273)

namespace SystemVueModelBuilder
{
	// classes defined in this module
	class FixedPointValue;

	// forward class declarations
	class FixedPointRep;
	class FixedPointParameters;
	class FixedPoint;



	// ----------------------------------------------------------------------------
	//  CLASS : FixedPointValue
	//
	//  Fixed-point value type; arbitrary precision fixed point type. An object of
	// FixedPointValue can store an arbitrary bit-width fixed point value with arbitrary
	// binary point location without loss of precision on magnitude i.e., no overflow or 
	// quantization handling is performed.
	// ----------------------------------------------------------------------------

	class SYSTEMC_FIXEDPOINT_API FixedPointValue
	{
		friend class FixedPoint;

	public:

		explicit FixedPointValue(  );
		FixedPointValue( int);
		FixedPointValue( unsigned int);
		FixedPointValue( long );
		FixedPointValue( unsigned long);
		FixedPointValue( double );
		FixedPointValue( const FixedPointValue& );
		FixedPointValue( const FixedPoint& );


		~FixedPointValue();


		// internal use only;
		const FixedPointRep*	get_rep() const;
		void					set_rep( FixedPointRep* );


		// unary operators

		const FixedPointValue  operator - () const;
		const FixedPointValue& operator + () const;


		// unary functions

		friend void neg( FixedPointValue&, const FixedPointValue& );


		// binary operators

		// FixedPointValue vs FixedPointValue
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, const FixedPointValue& );

		// FixedPointValue vs int
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( int, const FixedPointValue& );

		// FixedPointValue vs unsigned int
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, unsigned int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, unsigned int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, unsigned int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, unsigned int );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( unsigned int, const FixedPointValue& );

		// FixedPointValue vs long
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( long, const FixedPointValue& );

		// FixedPointValue vs unsigned long
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, unsigned long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, unsigned long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, unsigned long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, unsigned long );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( unsigned long, const FixedPointValue& );

		// FixedPointValue vs double
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( const FixedPointValue&, double );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator * ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( const FixedPointValue&, double );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator + ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( const FixedPointValue&, double );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator - ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( const FixedPointValue&, double );                
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator / ( double, const FixedPointValue& );


		// Shift Operator
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator << ( const FixedPointValue&, int );
		friend SYSTEMC_FIXEDPOINT_API const FixedPointValue operator >> ( const FixedPointValue&, int );



		// Logical functions
		friend SYSTEMC_FIXEDPOINT_API void lshift( FixedPointValue&, const FixedPointValue&, int );
		friend SYSTEMC_FIXEDPOINT_API void rshift( FixedPointValue&, const FixedPointValue&, int );



		// relational (including equality) operators
		// FixedPointValue& vs const FixedPointValue&
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, const FixedPointValue& );

		// relational (including equality) operators
		// FixedPointValue& vs const int
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( int, const FixedPointValue& );


		// relational (including equality) operators
		// FixedPointValue& vs const unsigned int
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, unsigned int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, unsigned int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, unsigned int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, unsigned int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, unsigned int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( unsigned int, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, unsigned int );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( unsigned int, const FixedPointValue& );



		// relational (including equality) operators
		// FixedPointValue& vs const long
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( long, const FixedPointValue& );


		// relational (including equality) operators
		// FixedPointValue& vs const unsigned long
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, unsigned long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, unsigned long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, unsigned long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, unsigned long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, unsigned long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( unsigned long, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, unsigned long );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( unsigned long, const FixedPointValue& );



		// relational (including equality) operators
		// FixedPointValue& vs const double
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( const FixedPointValue&, double );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator < ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( const FixedPointValue&, double );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator <= ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( const FixedPointValue&, double );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator > ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( const FixedPointValue&, double );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator >= ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( const FixedPointValue&, double );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator == ( double, const FixedPointValue& );
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( const FixedPointValue&, double );                          
		friend SYSTEMC_FIXEDPOINT_API bool operator != ( double, const FixedPointValue& );



		// assignment operators for int
		FixedPointValue& operator = ( int );
		FixedPointValue& operator *= ( int );
		FixedPointValue& operator /= ( int );
		FixedPointValue& operator += ( int );
		FixedPointValue& operator -= ( int );

		// assignment operators for unsigned int
		FixedPointValue& operator = ( unsigned int );
		FixedPointValue& operator *= ( unsigned int );
		FixedPointValue& operator /= ( unsigned int );
		FixedPointValue& operator += ( unsigned int );
		FixedPointValue& operator -= ( unsigned int );


		// assignment operators for long
		FixedPointValue& operator =  ( long );
		FixedPointValue& operator *= ( long );
		FixedPointValue& operator /= ( long );
		FixedPointValue& operator += ( long );
		FixedPointValue& operator -= ( long );


		// assignment operators for unsigned long
		FixedPointValue& operator =  ( unsigned long );
		FixedPointValue& operator *= ( unsigned long );
		FixedPointValue& operator /= ( unsigned long );
		FixedPointValue& operator += ( unsigned long );
		FixedPointValue& operator -= ( unsigned long );


		// assignment operators for double
		FixedPointValue& operator =  ( double );
		FixedPointValue& operator *= ( double );
		FixedPointValue& operator /= ( double );
		FixedPointValue& operator += ( double );
		FixedPointValue& operator -= ( double );



		// assignment operators for const FixedPointValue&
		FixedPointValue& operator =  ( const FixedPointValue& );
		FixedPointValue& operator *= ( const FixedPointValue& );
		FixedPointValue& operator /= ( const FixedPointValue& );
		FixedPointValue& operator += ( const FixedPointValue& );
		FixedPointValue& operator -= ( const FixedPointValue& );


		// assignment operators for const FixedPoint&
		FixedPointValue& operator =  ( const FixedPoint& );
		FixedPointValue& operator *= ( const FixedPoint& );
		FixedPointValue& operator /= ( const FixedPoint& );
		FixedPointValue& operator += ( const FixedPoint& );
		FixedPointValue& operator -= ( const FixedPoint& );


		FixedPointValue& operator <<= ( int );
		FixedPointValue& operator >>= ( int );





		// auto-increment and auto-decrement

		const FixedPointValue operator ++ ( int );
		const FixedPointValue operator -- ( int );

		FixedPointValue& operator ++ ();
		FixedPointValue& operator -- ();


		// explicit conversion to primitive types

		short          to_short() const;
		unsigned short to_ushort() const;
		int            to_int() const;
		unsigned int   to_uint() const;
		long           to_long() const;
		unsigned long  to_ulong() const;
		float          to_float() const;
		double         to_double() const;


		

        const std::string to_dec() const;
        const std::string to_bin() const;
		const std::string to_oct() const;
        const std::string to_hex() const;


		// query value

		bool is_neg() const;
		bool is_zero() const;
		bool is_nan() const;
		bool is_inf() const;
		bool is_normal() const;

		bool rounding_flag() const;

		// internal use only;
		FixedPointValue( FixedPointRep* );

		// Internal use only : explicit conversion to character string
		const std::string to_string() const;

		// Internal use only : explicit conversion to character string
		const std::string to_string( FixedPointEnums::NumRep ) const;

		// Internal use only : explicit conversion to character string
		const std::string to_string( FixedPointEnums::NumRep, bool ) const;

		// Internal use only : explicit conversion to character string
		const std::string to_string( FixedPointEnums::StringFormat ) const;

		// Internal use only : explicit conversion to character string
		const std::string to_string( FixedPointEnums::NumRep, FixedPointEnums::StringFormat ) const;

		// Internal use only : explicit conversion to character string
		const std::string to_string( FixedPointEnums::NumRep, bool, FixedPointEnums::StringFormat ) const;

	private:
		// internal use only;
		
		bool get_bit( int ) const;
		void get_type( int&, int&, FixedPointEnums::Sign& ) const;

		const FixedPointValue quantization( const FixedPointParameters&, bool& ) const;
		const FixedPointValue     overflow( const FixedPointParameters&, bool& ) const;

	private:

		FixedPointRep*		m_rep;


	};



} // namespace SystemVueModelBuilder
