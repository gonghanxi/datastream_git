// Copyright 2011 - 2014 Keysight Technologies, Inc
#pragma once

#include "SystemVue/DLL_Export/SystemC-FixedPoint.h"

namespace SystemVueModelBuilder
{

    //Forward declaration
    class FixedPoint;


    // ----------------------------------------------------------------------------
    //  CLASS : FixedPointBitRef
    //
    //  Proxy class for bit-selection in class FixedPoint, behaves like a bit.
    //	Provides reference to individual bits in a FixedPoint data type
    //
    //	An object of class FixedPointBitRef must not be created explicitly.
    //	The [] operator of a FixedPoint class returns an object of the
    //	FixedPointBitRef and that should be the only way to obtain an object
    //	of this class.
    // ----------------------------------------------------------------------------

    class SYSTEMC_FIXEDPOINT_API FixedPointBitRef
    {
        friend class FixedPoint;

    public:

        /// Copy Constructor
        FixedPointBitRef( const FixedPointBitRef& a );

        /// Assignment operator when RHS is another FixedPointBitRef
        FixedPointBitRef& operator = ( const FixedPointBitRef& a );

        /// Assignment operator when RHS is a bool
        FixedPointBitRef& operator = ( bool a );

        /// Unary AND (&) operator when RHS is a FixedPointBitRef
        FixedPointBitRef&	operator &= ( const FixedPointBitRef& b );

        /// Unary AND (&) operator when RHS is a bool
        FixedPointBitRef& operator &= ( bool b );

        /// Unary OR (|) operator when RHS is a FixedPointBitRef
        FixedPointBitRef& operator |= ( const FixedPointBitRef& b );

        /// Unary OR (|) operator when RHS is a bool
        FixedPointBitRef& operator |= ( bool b );

        /// Unary XOR (^) operator when RHS is a FixedPointBitRef
        FixedPointBitRef& operator ^= ( const FixedPointBitRef& b );

        /// Unary XOR (^) operator when RHS is a bool
        FixedPointBitRef& operator ^= ( bool b );

        /// Implicit conversion to bool
        operator bool() const;

    private:

        FixedPoint& m_num;
        int  m_idx;
        bool get() const;
        void set(bool val);

        // constructor

        inline FixedPointBitRef( FixedPoint& num_, int idx_ )
            : m_num( num_ ), m_idx( idx_ )
        {}
        // disabled
        FixedPointBitRef();


    };

    inline FixedPointBitRef::FixedPointBitRef( const FixedPointBitRef& a )
        : m_num( a.m_num ), m_idx( a.m_idx )
    {}


    /// Assignment operator when RHS is another FixedPointBitRef
    inline FixedPointBitRef& FixedPointBitRef::operator = ( const FixedPointBitRef& a )
    {
        if( &a != this )
        {
            set( a.get() );
        }
        return *this;
    }


    /// Assignment operator when RHS is a bool
    inline FixedPointBitRef& FixedPointBitRef::operator = ( bool a )
    {
        set( a );
        return *this;
    }

    /// Unary AND (&) operator when RHS is a FixedPointBitRef
    inline 	FixedPointBitRef&	FixedPointBitRef::operator &= ( const FixedPointBitRef& b )
    {
        set( get() && b.get() );
        return *this;
    }

    /// Unary AND (&) operator when RHS is a bool
    inline FixedPointBitRef& FixedPointBitRef::operator &= ( bool b )
    {
        set( get() && b );
        return *this;
    }

    /// Unary OR (|) operator when RHS is a FixedPointBitRef
    inline FixedPointBitRef& FixedPointBitRef::operator |= ( const FixedPointBitRef& b )
    {
        set( get() || b.get() );
        return *this;
    }


    /// Unary OR (|) operator when RHS is a bool
    inline FixedPointBitRef& FixedPointBitRef::operator |= ( bool b )
    {
        set( get() || b );
        return *this;
    }


    /// Unary XOR (^) operator when RHS is a FixedPointBitRef
    inline FixedPointBitRef& FixedPointBitRef::operator ^= ( const FixedPointBitRef& b )
    {
        set( get() != b.get() );
        return *this;
    }

    /// Unary XOR (^) operator when RHS is a bool
    inline FixedPointBitRef& FixedPointBitRef::operator ^= ( bool b )
    {
        set( get() != b );
        return *this;
    }


} // namespace SystemVueModelBuilder
