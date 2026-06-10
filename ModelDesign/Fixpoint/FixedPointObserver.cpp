// Copyright 2011 - 2014 Keysight Technologies, Inc
#include "FixedPointObserver.h"
#include "FixedPoint.h"
#include <cstring>
#include <cmath>

namespace SystemVueModelBuilder
{
    // DataObserverBase implementation
    DataObserverBase::DataObserverBase()
        : m_observedData(nullptr)
        , m_dMin(1e308)
        , m_dMax(-1e308)
        , m_iUpdateCount(0)
        , m_iZerosCount(0)
        , m_enuType(DataObserver::NOT_SET)
        , m_csName(nullptr)
        , m_piHistogramData(nullptr)
    {
        m_piHistogramData = new size_t[2048]();
    }

    DataObserverBase::~DataObserverBase()
    {
        delete[] m_csName;
        delete[] m_piHistogramData;
    }

    void DataObserverBase::SetObserver(void* observedData, const char* csName) {
        m_observedData = observedData;

        if (m_csName) {
            delete[] m_csName;
        }
        if (csName) {
            size_t len = strlen(csName) + 1;
            m_csName = new char[len];
            strcpy_s(m_csName, len, csName);
        }
    }

    void DataObserverBase::SetObservedData(void* observedData) {
        m_observedData = observedData;
    }

    void DataObserverBase::Update(double dData) {
        m_iUpdateCount++;

        if (dData == 0.0) {
            m_iZerosCount++;
        }

        if (dData < m_dMin) m_dMin = dData;
        if (dData > m_dMax) m_dMax = dData;

        // Update histogram
        if (dData != 0.0) {
            double absVal = fabs(dData);
            int msbPos = static_cast<int>(floor(log2(absVal)));
            int histIdx = msbPos + 1024;
            if (histIdx >= 0 && histIdx < 2048) {
                m_piHistogramData[histIdx]++;
            }
        }
    }

    size_t DataObserverBase::GetUnderflows() const { return 0; }
    size_t DataObserverBase::GetOverflows() const { return 0; }
    int DataObserverBase::GetWordLength() const { return 0; }
    int DataObserverBase::GetIntegerWordLength() const { return 0; }
    FixedPointEnums::Sign DataObserverBase::GetSign() { return FixedPointEnums::TWOS_COMPLEMENT; }

    double DataObserverBase::GetMin() { return m_dMin; }
    double DataObserverBase::GetMax() { return m_dMax; }
    size_t DataObserverBase::GetZeroCount() { return m_iZerosCount; }
    size_t DataObserverBase::GetUpdateCount() { return m_iUpdateCount; }
    const char* DataObserverBase::GetName() const { return m_csName; }
    const size_t* DataObserverBase::GetHistogramData() const { return m_piHistogramData; }

    void DataObserverBase::setDataType(DataObserver::Type type) { m_enuType = type; }
    DataObserver::Type DataObserverBase::getDataType() { return m_enuType; }

    // FixedPointObserver implementation
    FixedPointObserver::FixedPointObserver()
        : m_iUnderFlows(0)
        , m_iOverFlows(0)
    {
        setDataType(DataObserver::FIXED_POINT);
    }

    void FixedPointObserver::SetObserver(const FixedPoint* observedData, const char* csName) {
        DataObserverBase::SetObserver(const_cast<FixedPoint*>(observedData), csName);
    }

    void FixedPointObserver::Update() {
        const FixedPoint* fpData = static_cast<const FixedPoint*>(GetDataPointer());
        if (!fpData) return;

        double value = fpData->to_double();
        DataObserverBase::Update(value);

        // Check for overflow/underflow
        if (fpData->overflow_flag()) {
            m_iOverFlows++;
        }
        if (fpData->quantization_flag()) {
            m_iUnderFlows++;
        }
    }

    size_t FixedPointObserver::GetUnderflows() const { return m_iUnderFlows; }
    size_t FixedPointObserver::GetOverflows() const { return m_iOverFlows; }

    int FixedPointObserver::GetWordLength() const {
        const FixedPoint* fpData = static_cast<const FixedPoint*>(GetDataPointer());
        return fpData ? fpData->wl() : 0;
    }

    int FixedPointObserver::GetIntegerWordLength() const {
        const FixedPoint* fpData = static_cast<const FixedPoint*>(GetDataPointer());
        return fpData ? fpData->iwl() : 0;
    }

    FixedPointEnums::Sign FixedPointObserver::GetSign() {
        const FixedPoint* fpData = static_cast<const FixedPoint*>(GetDataPointer());
        return fpData ? fpData->sign() : FixedPointEnums::TWOS_COMPLEMENT;
    }

    // IntObserver implementation
    void IntObserver::SetObserver(const int* observedData, const char* csName) {
        DataObserverBase::SetObserver(const_cast<int*>(observedData), csName);
        setDataType(DataObserver::INTEGER);
    }

    void IntObserver::Update() {
        const int* intData = static_cast<const int*>(GetDataPointer());
        if (!intData) return;
        DataObserverBase::Update(static_cast<double>(*intData));
    }

    int IntObserver::GetWordLength() const { return 32; }
    int IntObserver::GetIntegerWordLength() const { return 32; }

    // DoubleObserver implementation
    void DoubleObserver::SetObserver(const double* observedData, const char* csName) {
        DataObserverBase::SetObserver(const_cast<double*>(observedData), csName);
        setDataType(DataObserver::DOUBLE);
    }

    void DoubleObserver::Update() {
        const double* doubleData = static_cast<const double*>(GetDataPointer());
        if (!doubleData) return;
        DataObserverBase::Update(*doubleData);
    }

    int DoubleObserver::GetWordLength() const { return 64; }
    int DoubleObserver::GetIntegerWordLength() const { return 32; }

} // namespace SystemVueModelBuilder
