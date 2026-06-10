// Copyright 2011 - 2014 Keysight Technologies, Inc
#include "FixedPointParameters.h"

namespace SystemVueModelBuilder
{

    // Default Constructor
    FixedPointParameters::FixedPointParameters()
        : m_sign(FixedPointEnums::TWOS_COMPLEMENT)
        , m_wl(FixedPointEnums::DEFAULT_WL_)
        , m_iwl(FixedPointEnums::DEFAULT_IWL_)
        , m_saturationBits(FixedPointEnums::DEFAULT_N_BITS_)
        , m_quantizationMode(FixedPointEnums::DEFAULT_QUANTIZATION_MODE)
        , m_overflowMode(FixedPointEnums::DEFAULT_OVERFLOW_MODE)
    {
    }

    // Copy Constructor
    FixedPointParameters::FixedPointParameters(const FixedPointParameters &rhs)
        : m_sign(rhs.m_sign)
        , m_wl(rhs.m_wl)
        , m_iwl(rhs.m_iwl)
        , m_saturationBits(rhs.m_saturationBits)
        , m_quantizationMode(rhs.m_quantizationMode)
        , m_overflowMode(rhs.m_overflowMode)
    {
    }

    // Overloaded constructor
    FixedPointParameters::FixedPointParameters(int wl, int iwl, FixedPointEnums::Sign eSign,
        FixedPointEnums::QuantizationMode qm, FixedPointEnums::OverflowMode om, int nb)
        : m_sign(eSign)
        , m_wl(wl)
        , m_iwl(iwl)
        , m_saturationBits(nb)
        , m_quantizationMode(qm)
        , m_overflowMode(om)
    {
    }

    void FixedPointParameters::setParameters(int wl, int iwl, FixedPointEnums::Sign eSign,
        FixedPointEnums::QuantizationMode qm, FixedPointEnums::OverflowMode om, int nb)
    {
        m_wl = wl;
        m_iwl = iwl;
        m_sign = eSign;
        m_quantizationMode = qm;
        m_overflowMode = om;
        m_saturationBits = nb;
    }

    void FixedPointParameters::setParameters(FixedPointEnums::Sign eSign,
        FixedPointEnums::QuantizationMode qm, FixedPointEnums::OverflowMode om, int nb)
    {
        m_sign = eSign;
        m_quantizationMode = qm;
        m_overflowMode = om;
        m_saturationBits = nb;
    }

    FixedPointEnums::Sign FixedPointParameters::sign() const
    {
        return m_sign;
    }

    int FixedPointParameters::wl() const
    {
        return m_wl;
    }

    int FixedPointParameters::iwl() const
    {
        return m_iwl;
    }

    int FixedPointParameters::fwl() const
    {
        return m_wl - m_iwl;
    }

    FixedPointEnums::QuantizationMode FixedPointParameters::q_mode() const
    {
        return m_quantizationMode;
    }

    FixedPointEnums::OverflowMode FixedPointParameters::o_mode() const
    {
        return m_overflowMode;
    }

    int FixedPointParameters::saturationBits() const
    {
        return m_saturationBits;
    }

    bool FixedPointParameters::operator==(FixedPointParameters& cfpX)
    {
        return (m_sign == cfpX.m_sign) &&
               (m_wl == cfpX.m_wl) &&
               (m_iwl == cfpX.m_iwl) &&
               (m_saturationBits == cfpX.m_saturationBits) &&
               (m_quantizationMode == cfpX.m_quantizationMode) &&
               (m_overflowMode == cfpX.m_overflowMode);
    }

} // namespace SystemVueModelBuilder
