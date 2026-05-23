#ifndef TIMEDCIRCULARBUFFER_H
#define TIMEDCIRCULARBUFFER_H

#include "CircularBuffer.h"
#include "Matrix.h"

namespace SystemVueModelBuilder {
    class CircularBufferTime
    {
    public:
        CircularBufferTime() { m_dStartTime = m_dTimeStep = m_dSampleRate = 0; }
        /// Get time stamp
        double GetTime( size_t iIndex, unsigned long long iCount, size_t iRate, size_t iHistroyDepth ) const
        {
            return ( iCount * iRate + iIndex - ( iHistroyDepth - iRate ) ) * m_dTimeStep + m_dStartTime;
        }
        /// Set sample rate
        bool SetSampleRate( double dSampleRate )
        {
            bool bStatus = false;
            _ASSERT( dSampleRate >=0 );
            if ( dSampleRate >= 0 )
            {
                m_dSampleRate = dSampleRate;
                if ( dSampleRate > 0)
                {
                    m_dTimeStep = 1.0 / m_dSampleRate;
                }
                else
                {
                    m_dTimeStep = 0;
                }
                bStatus = true;
            }
            return bStatus;
        }

        /// Set time step
        bool SetTimeStep( double dTimeStep )
        {
            bool bStatus = false;
            _ASSERT( dTimeStep > 0 );
            if ( dTimeStep > 0 )
            {
                m_dTimeStep = dTimeStep;
                m_dSampleRate = 1.0 / m_dTimeStep;
                bStatus = true;
            }
            return bStatus;
        }
        /// Set the time stamp of the first sample.
        inline void SetStartTime( double dStartTime ) { m_dStartTime = dStartTime; }

        /// Get sample rate.
        inline double GetSampleRate() const { return m_dSampleRate; }

        /// Get time step.
        inline double GetTimeStep() const { return m_dTimeStep; }

        /// Get start time.
        inline double GetStartTime() const { return m_dStartTime; }
    private:
        double m_dStartTime;
        double m_dTimeStep;
        double m_dSampleRate;
    };

    /// TimedCircularBuffer
    template<typename T> class TimedCircularBuffer : public CircularBuffer< T >
    {
    public:
        TimedCircularBuffer< T >() : CircularBuffer< T >() {}
        /// Get the time stamp at the (iCount)th firing of the model and the (iIndex)th sample of the buffer.
        /// Use this method in TimedDFModel::Run to get the time stamp of a particular sample.
        double GetTime( size_t iIndex, unsigned long long iCount ) const
        {
            return m_Time.GetTime( iIndex, iCount, CircularBuffer< T >::m_iRate, CircularBuffer< T >::m_iHistoryDepth );
        }

        /// Set sample rate and the corresponding time step ( 1 / sample rate ).
        /// Use this method in TimedDFModel::Setup to set the sample rate of this buffer.
        bool SetSampleRate( double dSampleRate ) { return m_Time.SetSampleRate( dSampleRate ); }

        /// Set time step and the corresponding sample rate ( 1 / time step ).
        /// Use this method in TimedDFModel::Setup to set the sample rate of this buffer.
        bool SetTimeStep( double dTimeStep ) { return m_Time.SetTimeStep( dTimeStep ); }

        /// Set start time of this buffer
        /// Use this method in TimedDFModel::CalculateLatency to set the start time of output buffer.
        void SetStartTime( double dStartTime ) { return m_Time.SetStartTime( dStartTime ); }

        /// Get sample rate
        double GetSampleRate() const { return m_Time.GetSampleRate(); }

        /// Get time step
        double GetTimeStep() const { return m_Time.GetTimeStep(); }

        /// Get Start time
        double GetStartTime() const { return m_Time.GetStartTime(); }

        /// Timing property
        CircularBufferTime m_Time;
    };

    /// TimedCircularBufferE
    template<typename T> class TimedCircularBufferE : public CircularBufferE< T >
    {
    public:
        /// Constructor
        TimedCircularBufferE< T >() : CircularBufferE< T >() {}

        /// Get the time stamp at the (iCount)th firing of the model and the (iIndex)th sample of the buffer.
        /// Use this method in TimedDFModel::Run to get the time stamp of a particular sample.
        double GetTime( size_t iIndex, unsigned long long iCount ) const
        {
            return m_Time.GetTime( iIndex, iCount, CircularBufferE< T >::m_iRate, CircularBufferE< T >::m_iHistoryDepth );
        }

        /// Set sample rate and the corresponding time step ( 1 / sample rate ).
        /// Use this method in TimedDFModel::Setup to set the sample rate of this buffer.
        bool SetSampleRate( double dSampleRate ) { return m_Time.SetSampleRate( dSampleRate ); }

        /// Set time step and the corresponding sample rate ( 1 / time step ).
        /// Use this method in TimedDFModel::Setup to set the sample rate of this buffer.
        bool SetTimeStep( double dTimeStep ) { return m_Time.SetTimeStep( dTimeStep ); }

        /// Set start time of this buffer
        /// Use this method in TimedDFModel::CalculateLatency to set the start time of output buffer.
        void SetStartTime( double dStartTime ) { return m_Time.SetStartTime( dStartTime ); }

        /// Get sample rate
        double GetSampleRate() const { return m_Time.GetSampleRate(); }

        /// Get time step
        double GetTimeStep() const { return m_Time.GetTimeStep(); }

        /// Get Start time
        double GetStartTime() const { return m_Time.GetStartTime(); }

        /// Timing property
        CircularBufferTime m_Time;
    };
    }
#endif // TIMEDCIRCULARBUFFER_H
