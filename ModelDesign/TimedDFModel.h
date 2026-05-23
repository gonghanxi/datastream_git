#ifndef TIMEDDFMODEL_H
#define TIMEDDFMODEL_H
#include "DFModel.h"
#include "eresult.h"

//-----------------------------------------------------------------------------------
//
//	TimedDFModel.h
//		Definition of TimedDFModel class - the base class for timed C++ Data Flow models
//-----------------------------------------------------------------------------------
namespace SystemVueModelBuilder {
    /// Timed DFModel
    class  TimedDFModel : public DFModel
    {
    public:
        /// Constructor
        TimedDFModel() : DFModel() { m_iFiringCount = 0; }

        /// Destructor
        virtual ~TimedDFModel() {}

        /// Advance count after each firing.
        inline void Advance() { m_iFiringCount++; }

        /// Get the current firing count.
        inline unsigned long long GetCount() { return m_iFiringCount; }

        /// Set the start time of output TimedCircularBuffer
        /// based on the start time and time step of input TimedCircularBuffer and model parameters.
        /// This method allows users to incorporate latency into the model's timing behavior.
        /// If the derived model does not override this method, the start time of the output is default to the start time of the input.
        virtual ERESULT CalculateLatency() { return E_NOTIMPL_; }

        /// Set the characterization frequency of output EnvelopeCircularBuffer
        /// based on the characterization frequency of input EnvelopeCircularBuffer and model parameters.
        /// If the derived model does not override this method, the characterization frequency of output EnvelopeCircularBuffer
        /// is default to the maximum characterization frequency among input EnvelopeCircularBuffers.
        virtual ERESULT PropagateCharacterizationFrequency() { return E_NOTIMPL_; }

    protected:
        /// Firing count
        unsigned long long m_iFiringCount;
    };
    }

#endif // TIMEDDFMODEL_H
