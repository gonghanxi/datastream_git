#ifndef ENVELOPESIGNAL_H
#define ENVELOPESIGNAL_H

#include "TimedCircularBuffer.h"
#include "Matrix.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace SystemVueModelBuilder {
class EnvelopeSignal
{
public:
    /// Default constructor
    EnvelopeSignal() { m_cxSignal = 0; }

    /// Convert constructor
    EnvelopeSignal( double d ) { m_cxSignal = std::complex<double>(d); }

    /// Convert constructor
    EnvelopeSignal( const std::complex<double>& cx ) { m_cxSignal = cx; }

    /// Get real part
    /// If the characterization frequency is 0, use this method to get the real baseband signal value
    inline double real() const { return m_cxSignal.real(); }

    /// Get imaginary part
    inline double imag() const { return m_cxSignal.imag(); }

    /// Get complex value
    /// If the characterization frequency is greater than 0, use this method to get the complex envelope I-Q value
    inline std::complex<double> complex() const { return m_cxSignal; }

    /// Assignment operator for std::complex<double>
    /// Use this method to assign complex envelope I-Q value to the EnvelopeSignal
    EnvelopeSignal& operator = ( const std::complex<double>& cx ) { m_cxSignal = cx; return *this; }

    /// Assignment operator for double.
    /// Use this method to assign real baseband value to the EnvelopeSignal.
    EnvelopeSignal& operator = ( const double& d ) { m_cxSignal = d; return *this; }

    bool operator == (const EnvelopeSignal& env) const { return m_cxSignal == env.m_cxSignal; }

    /// Convert complex envelope I-Q representation to real baseband signal
    /// Use this method only if the characterization frequency is greater than 0
    /// \param dFc The characterization frequency
    /// \param dTime The time stamp of the EnvelopeSignal, which can be obtained from TimedCircularBuffer
    /// \return The converted baseband real signal value of the complex envelope
    double ConvertToReal( double dFc, double dTime ) const
    {
        return m_cxSignal.real() * cos( 2.0 * M_PI * dFc * dTime ) - m_cxSignal.imag() * sin( 2.0 * M_PI * dFc * dTime );
    }

    /// Convert complex envelope I-Q value characterized at dFc to the equivalent I-Q representation at characterization frequency dNewFc
    /// Use this method only if the characterization frequency is greater than 0
    /// \param dFc The characterization frequency of the complex envelope
    /// \param dNewFc The new characterization frequency
    /// \param dTime The time stamp of the EnvelopeSignal, which can be obtained from TimedCircularBuffer
    /// \return The converted complex envelope I-Q value characterized at dNewFc
    std::complex<double> ConvertToNewFc( double dFc, double dNewFc, double dTime ) const
    {
        return m_cxSignal * std::complex<double>( cos( 2.0 * M_PI * (dFc - dNewFc) * dTime ), sin( 2.0 * M_PI* (dFc - dNewFc) * dTime ) );
    }

    /// Add and assignment operator
    /// Use this method to add an EnvelopeSignal to this EnvelopeSignal assuming the characterization frequency is the same.
    EnvelopeSignal& operator += (const EnvelopeSignal& env) { m_cxSignal += env.m_cxSignal; return *this; }

    /// Add operator
    /// Use this method to add an EnvelopeSignal to this EnvelopeSignal assuming the characterization frequency is the same.
    EnvelopeSignal operator + (const EnvelopeSignal& env) const { return EnvelopeSignal( m_cxSignal + env.m_cxSignal ); }

    /// Add and assignment operator for std::complex<double>.
    /// Use this method to add a complex value to the EnvelopeSignal.
    EnvelopeSignal& operator += (const std::complex<double>& cx) { m_cxSignal += cx; return *this; }

    /// Add operator for std::complex<double>.
    /// Use this method to add a complex value to the EnvelopeSignal.
    EnvelopeSignal operator + (const std::complex<double>& cx) const { return EnvelopeSignal( m_cxSignal + cx ); }

    /// Multiply and assignment operator for std::complex<double>.
    /// Use this method to multiply a complex value to the EnvelopeSignal.
    EnvelopeSignal& operator *= (const std::complex<double>& cx) { m_cxSignal *= cx; return *this; }

    /// Multiply operator for std::complex<double>.
    /// Use this method to multiply a complex value to the EnvelopeSignal.
    EnvelopeSignal operator * (const std::complex<double>& cx) const { return EnvelopeSignal( m_cxSignal * cx ); }

    /// Devide and assignment operator for double.
    /// Use this method to devide a real value from the EnvelopeSignal.
    EnvelopeSignal& operator /= (const double& d) { m_cxSignal /= d; return *this; }

    /// Devide operator for double.
    /// Use this method to devide a real value from the EnvelopeSignal.
    EnvelopeSignal operator / (const double& d) const { return EnvelopeSignal( m_cxSignal / d ); }

private:
    std::complex<double> m_cxSignal;
};

typedef Matrix<EnvelopeSignal> EnvelopeMatrix;

template <class CIRCULAR_BUFFER, class ENV_SIGNAL> class EnvelopeCircularBufferT : public CIRCULAR_BUFFER
{
public:
    /// Constructor
    /// Characterization frequency is default to 0.
    EnvelopeCircularBufferT() : CIRCULAR_BUFFER() { m_dFc = 0; }

    /// Set characterization frequency
    inline void SetCharacterizationFrequency( double dFc ) { _ASSERT( dFc >= 0 || dFc ==-1 ); m_dFc = dFc; }

    /// Get characterization frequency
    inline double GetCharacterizationFrequency() const { return m_dFc; }

private:
    /// Characterization frequency
    double m_dFc;
};

typedef EnvelopeCircularBufferT < TimedCircularBuffer<EnvelopeSignal>, EnvelopeSignal > EnvelopeCircularBuffer;
typedef EnvelopeCircularBufferT < TimedCircularBufferE<EnvelopeMatrix>, EnvelopeMatrix > EnvelopeMatrixCircularBuffer;

/// Circular buffer bus for EnvelopeSignal
typedef CircularBufferBusT<EnvelopeCircularBuffer> EnvelopeCircularBufferBus;
typedef CircularBufferBusT<EnvelopeMatrixCircularBuffer> EnvelopeMatrixCircularBufferBus;

inline void CopyToEnvelopeSignal(const std::complex<double>& input, EnvelopeSignal& output)
{
    output = input;
}

inline void CopyToEnvelopeSignal(const Matrix<std::complex<double>>& input, EnvelopeMatrix& output)
{
    output.Resize(input.NumRows(), input.NumColumns());
    if (input.NumElements() > 0)
        memcpy(output.GetBuffer(), input.GetBuffer(), input.NumElements() * sizeof(std::complex<double>));
}

inline void CopyRealToEnvelopeSignal(const std::complex<double>& input, EnvelopeSignal& output)
{
    output = input.real();
}

inline void CopyRealToEnvelopeSignal(const Matrix<std::complex<double>>& input, EnvelopeMatrix& output)
{
    output.Resize(input.NumRows(), input.NumColumns());
    for (size_t i = 0; i < input.NumElements(); i++)
    {
        output(i) = input(i).real();
    }
}

inline void CopyFromEnvelopeSignal(const EnvelopeSignal& input, std::complex<double>& output)
{
    output = input.complex();
}

inline void CopyFromEnvelopeSignal(const EnvelopeMatrix& input, Matrix<std::complex<double>>& output)
{
    output.Resize(input.NumRows(), input.NumColumns());
    if (input.NumElements() > 0)
        memcpy(output.GetBuffer(), input.GetBuffer(), input.NumElements() * sizeof(std::complex<double>));
}



}
#endif // ENVELOPESIGNAL_H
