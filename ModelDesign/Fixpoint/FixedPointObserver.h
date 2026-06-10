// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

#include <crtdbg.h>
#include "SystemVue/FixedPointEnums.h"
#include "SystemVue/DLL_Export/SystemC-FixedPoint.h"
#include <cstddef>

namespace SystemVueModelBuilder
{
	//Forward Declaration
	class FixedPoint;


	namespace DataObserver
	{
	/// The following enumerations are for internal use only
		enum Type
		{
			NOT_SET,
			FIXED_POINT,
			INTEGER,
			DOUBLE,
			FIXED_POINT_CIRCULAR_BUFFER,
			DOUBLE_CIRCULAR_BUFFER,
			INTEGER_CIRCULAR_BUFFER
		};
	}


	/// DataObserver Base class used in fixed point analysis data collection
	/// This is an abstract class.
	/// For corresponding classes see FixedPointObserver, IntegerObserver, and DoubleObserver
	class  SYSTEMC_FIXEDPOINT_API DataObserverBase
	{
	public:

		/// Default constructor
		DataObserverBase();
		
		/// Destructor
		~DataObserverBase();


		/// Virtual Update method
		/// Defined in derived classes
		virtual void Update()=0;

		/// Virtual Accessor method to return Underflows
		/// For int and double there will be no under flows 
		/// by default. For fixed point this functions is 
		/// overridden in FixedPointObserver class
		virtual size_t GetUnderflows() const;
		

		/// Virtual Accessor method to return Overflows
		/// For int and double there will be no over flows 
		/// by default. For fixed point this functions is 
		/// overridden in FixedPointObserver class
		virtual size_t GetOverflows() const;
		
		/// Get word length of the data type being observed
		virtual int GetWordLength() const;

		/// Get word length of the data type being observed
		virtual int GetIntegerWordLength() const;

		//// Returns the data being observed is of SystemVueModelBuilder::FixedPointEnums::UNSIGNED type
		/// or SystemVueModelBuilder::FixedPointEnums::TWOS_COMPLEMENT type
		virtual FixedPointEnums::Sign GetSign();

		/// Returns the minimum data value found 
		double GetMin();
		
		/// Returns the maximum data value found 
		double GetMax();
		
		
		/// Returns number of zeros observed
		size_t GetZeroCount();

		/// Returns number of times update was called
		size_t GetUpdateCount();
		

		/// Returns the name of the observer, as shown in fixed point
		/// analysis table
		const char * GetName() const;



		/// Returns reference to size_t * containing the histogram data.
		/// The value at vector index 0 represents number of absolute data values 
		/// where MSB which is '1' is at bit location -1024 (i.e. a fractional bit). 
		/// The value at index 2047 represents number of abosulte data values 
		/// where MSB which is '1' is at bit location 1023 (i.e. an integral bit). 
		/// The value at index 1024 represents number of abosulte data values 
		/// where MSB which is '1' is at bit location 0 (i.e. first integral bit).
		/// The size of returned pointer is 2048, accessing the returned pointer may
		/// cause un-expected behavior.
		const size_t * GetHistogramData() const;
		

		
	protected:

		/// Internal use only
		void SetObserver(void * observedData, const char * csName) ;


		/// internal use only
		
		void SetObservedData(void * observedData);
		
		
		/// Internal use only
		void Update(double dData);

		/// Internal use only
		inline  const void * GetDataPointer() const
		{
			return m_observedData;
		}

		/// Internal use only
		void setDataType(DataObserver::Type type);

		/// Internal use only
		DataObserver::Type getDataType();


	private:

		/// pointer to data being observed
		void * m_observedData;
		
		/// Minimum value found so far 
		double m_dMin;

		/// Maximum value found so far
		double m_dMax;

		/// Number of times update was called
		size_t m_iUpdateCount;

		/// Number of times data==0
		size_t m_iZerosCount;

		

		/// Type of the data being observed
		DataObserver::Type m_enuType;

	protected:
			/// Name of observer as it will appear in fixed point analysis table
		char * m_csName; 

		/// Histogram data 
		size_t * m_piHistogramData;
	};



	/// Fixed point observer class to observe data which is internal to a model
	/// Input/output ports are observed automatically
	class SYSTEMC_FIXEDPOINT_API FixedPointObserver : public DataObserverBase
	{
	public:

		/// Default constructor
		FixedPointObserver( );


		/// Sets the fixed point data to be observed by the observer, this must be called 
		/// before calling the Update() method. Also the data being observed must be a 
		/// class data member, local variables must not be observed because those do not 
		/// span the whole life time of class object.
		/// <param name="observedData"> pointer to data being observed. </param>
		/// <param name="csName"> the name of data as it should appear on fixed point 
		/// analysis table </param>
		/// See documentation for further details.
		void SetObserver(const FixedPoint * observedData, const char * csName);
		


		/// Sets the pointer to data to be obsereved. This method can be used when 
		/// you would like to update the data being observed for this observer.
		/// The SetObserver method must be called at least once prior to calling this method
		/// <param name="observedData"> pointer to data being observed. </param>
		inline void SetObservedData(const FixedPoint * observedData)
		{
			/// Cannot observe a data pointed by a null pointer
			_ASSERT(observedData);

			// You must call SetObserver method to set observre name before calling this 
			// method to set observer
			_ASSERT(m_csName);
			_ASSERT(m_piHistogramData);

			DataObserverBase::SetObservedData((void *)observedData);
		}


		/// The Update method must be called whenever there is a need to update
		/// the observer based on the current value of data being observed.
		/// The Update method must be called at least once. 
		void Update();


		/// Returns how many times an underflow was detected on the data being observed, 
		/// this value is always less than or equal to the number of times Update() method
		/// was called.
		size_t GetUnderflows() const;
		
		/// Returns how many times an overflow was detected on the data being observed, 
		/// this value is always less than or equal to the number of times Update() method
		/// was called.
		size_t GetOverflows() const;
		

		/// Get word length of the data being observed
		int GetWordLength() const;

		/// Get word length of the data being observed
		int GetIntegerWordLength() const;

		//// Returns the data being observed is of SystemVueModelBuilder::FixedPointEnums::UNSIGNED type
		/// or SystemVueModelBuilder::FixedPointEnums::TWOS_COMPLEMENT type
		FixedPointEnums::Sign GetSign();
		
	protected:
		/// number of overflows detected
		size_t m_iUnderFlows;

		/// numbre of underflows detected
		size_t m_iOverFlows;
	};


	/// Ineteger observer class to observe data which is internal to a model
	/// Input/output ports are observed automatically
	class SYSTEMC_FIXEDPOINT_API IntObserver : public DataObserverBase
	{
	public:

		/// Sets the data to be observed by the observer, this must be called before
		/// calling the Update() method. Also the data being observed must be a class
		/// data member, local variables must not be observed because those do not span
		/// the whole life time of class object.
		/// <param name="observedData"> pointer to data being observed. </param>
		/// <param name="csName"> the name of data as it should appear on fixed point 
		/// analysis table </param>
		/// See documentation for further details.
		void SetObserver(const int * observedData, const char * csName); 
		

		/// Sets the pointer to data to be obsereved. This method can be used when 
		/// you would like to update the data being observed for this observer.
		/// The SetObserver method must be called at least once prior to calling this method
		/// <param name="observedData"> pointer to data being observed. </param>
		inline void SetObservedData(const int * observedData)
		{
			/// Cannot observe a data pointed by a null pointer
			_ASSERT(observedData);
			
			// You must call SetObserver method to set observre name before calling this 
			// method to set observer
			_ASSERT(m_csName);
			_ASSERT(m_piHistogramData);
			DataObserverBase::SetObservedData((void *)observedData);
		}


		/// The Update method must be called whenever there is a need to update
		/// the observer based on the current value of data being observed.
		/// The Update method must be called at least once. 
		void Update();


		/// Get word length of the data being observed
		int GetWordLength() const;

		/// Get word length of the data being observed
		int GetIntegerWordLength() const;
	};

	/// Double observer class to observe data which is internal to a model
	/// Input/output ports are observed automatically
	class SYSTEMC_FIXEDPOINT_API DoubleObserver : public DataObserverBase
	{
	public:

		/// sets the data to be observed by the observer, this must be called before
		/// calling the Update() method. Also the data being observed must be a class
		/// data member, local variables must not be observed because those do not span
		/// the whole life time of class object.
		/// <param name="observedData"> pointer to data being observed. </param>
		/// <param name="csName"> the name of data as it should appear on fixed point 
		/// analysis table </param>
		/// See documentation for further details.
		void SetObserver(const double * observedData, const char * csName); 
		

		// Sets the pointer to data to be obsereved. This method can be used when 
		/// you would like to update the data being observed for this observer.
		/// The SetObserver method must be called at least once prior to calling this method
		/// <param name="observedData"> pointer to data being observed. </param>
		inline void SetObservedData(const double * observedData)
		{
			/// Cannot observe a data pointed by a null pointer
			_ASSERT(observedData);

			// You must call SetObserver method to set observre name before calling this 
			// method to set observer
			_ASSERT(m_csName);
			_ASSERT(m_piHistogramData);
			DataObserverBase::SetObservedData((void *)observedData);
		}

		/// The Update method must be called whenever there is a need to update
		/// the observer based on the current value of data being observed.
		/// The Update method must be called at least once. 
		void Update();

		/// Get word length of the data being observed
		int GetWordLength() const;

		/// Get word length of the data being observed
		int GetIntegerWordLength() const;
	};


}// end namespace SystemVueModelBuilder

