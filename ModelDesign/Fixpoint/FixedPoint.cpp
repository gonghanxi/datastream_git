// Copyright 2011 - 2014 Keysight Technologies, Inc
#include "FixedPoint.h"
#include "FixedPointValue.h"
#include "FixedPointParameters.h"
#include "FixedPointBitRef.h"
#include "FixedPointRep.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace SystemVueModelBuilder
{
    // Private implementation helper
    struct FixedPointImpl {
        static FixedPointRep* createRep(const FixedPointParameters& params) {
            return new FixedPointRep(0.0, params.wl(), params.iwl(), params.sign());
        }

        static void applyQuantization(double& value, const FixedPointParameters& params) {
            // Simplified quantization - truncate to fractional bits
            int fwl = params.wl() - params.iwl();
            if (fwl > 0) {
                double scale = pow(2.0, fwl);
                value = floor(value * scale) / scale;
            }
        }

        static void applyOverflow(double& value, const FixedPointParameters& params) {
            double maxVal, minVal;
            if (params.sign() == FixedPointEnums::UNSIGNED) {
                maxVal = pow(2.0, params.iwl()) - 1.0;
                minVal = 0.0;
            } else {
                maxVal = pow(2.0, params.iwl() - 1) - 1.0;
                minVal = -pow(2.0, params.iwl() - 1);
            }

            switch (params.o_mode()) {
            case FixedPointEnums::SATURATE:
                value = std::max(minVal, std::min(maxVal, value));
                break;
            case FixedPointEnums::SATURATE_ZERO:
                if (value > maxVal || value < minVal) value = 0.0;
                break;
            case FixedPointEnums::SATURATE_SYMMETRICAL:
                if (value > maxVal) value = maxVal;
                else if (value < -maxVal) value = -maxVal;
                break;
            case FixedPointEnums::WRAP:
                // Simplified wrap
                break;
            case FixedPointEnums::WRAP_SIGN_MAGNITUDE:
                // Simplified wrap sign magnitude
                break;
            }
        }
    };

    // Constructors
    FixedPoint::FixedPoint()
        : m_rep(new FixedPointRep())
        , m_quantizationFlag(false)
        , m_overflowFlag(false)
    {
    }

    FixedPoint::FixedPoint(int val)
        : m_rep(new FixedPointRep(static_cast<double>(val), 32, 32, FixedPointEnums::TWOS_COMPLEMENT))
        , m_quantizationFlag(false)
        , m_overflowFlag(false)
    {
        m_parameters.setParameters(32, 32, FixedPointEnums::TWOS_COMPLEMENT);
    }

    FixedPoint::FixedPoint(double val)
        : m_rep(new FixedPointRep(val, 64, 32, FixedPointEnums::TWOS_COMPLEMENT))
        , m_quantizationFlag(false)
        , m_overflowFlag(false)
    {
        m_parameters.setParameters(64, 32, FixedPointEnums::TWOS_COMPLEMENT);
    }

    FixedPoint::FixedPoint(const FixedPoint& other)
        : m_rep(new FixedPointRep(*other.m_rep))
        , m_parameters(other.m_parameters)
        , m_quantizationFlag(other.m_quantizationFlag)
        , m_overflowFlag(other.m_overflowFlag)
    {
    }

    FixedPoint::~FixedPoint()
    {
        delete m_rep;
    }

    bool FixedPoint::isSigned() const
    {
        return m_parameters.sign() == FixedPointEnums::TWOS_COMPLEMENT;
    }

    void FixedPoint::cast()
    {
        // Apply quantization and overflow based on parameters
        FixedPointImpl::applyQuantization(m_rep->value, m_parameters);
        FixedPointImpl::applyOverflow(m_rep->value, m_parameters);
    }

    // Conversion to FixedPointValue
    FixedPoint::operator FixedPointValue() const
    {
        return FixedPointValue(m_rep->value);
    }

    // Unary operators
    const FixedPointValue FixedPoint::operator-() const
    {
        return FixedPointValue(-m_rep->value);
    }

    const FixedPointValue FixedPoint::operator+() const
    {
        return FixedPointValue(m_rep->value);
    }

    const FixedPoint FixedPoint::operator~() const
    {
        FixedPoint result(*this);
        result.m_rep->value = ~static_cast<long long>(m_rep->value);
        return result;
    }

    // Binary operators FixedPoint vs FixedPoint
    const FixedPointValue operator*(const FixedPoint& a, const FixedPoint& b) {
        return FixedPointValue(a.to_double() * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, const FixedPoint& b) {
        return FixedPointValue(a.to_double() + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, const FixedPoint& b) {
        return FixedPointValue(a.to_double() - b.to_double());
    }

    // Binary operators FixedPoint vs FixedPointValue
    const FixedPointValue operator*(const FixedPoint& a, const FixedPointValue& b) {
        return FixedPointValue(a.to_double() * b.to_double());
    }

    const FixedPointValue operator*(const FixedPointValue& a, const FixedPoint& b) {
        return FixedPointValue(a.to_double() * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, const FixedPointValue& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b.to_double());
    }

    const FixedPointValue operator/(const FixedPointValue& a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, const FixedPointValue& b) {
        return FixedPointValue(a.to_double() + b.to_double());
    }

    const FixedPointValue operator+(const FixedPointValue& a, const FixedPoint& b) {
        return FixedPointValue(a.to_double() + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, const FixedPointValue& b) {
        return FixedPointValue(a.to_double() - b.to_double());
    }

    const FixedPointValue operator-(const FixedPointValue& a, const FixedPoint& b) {
        return FixedPointValue(a.to_double() - b.to_double());
    }

    // Binary operators FixedPoint vs int
    const FixedPointValue operator*(const FixedPoint& a, int b) {
        return FixedPointValue(a.to_double() * b);
    }

    const FixedPointValue operator*(int a, const FixedPoint& b) {
        return FixedPointValue(a * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, int b) {
        if (b == 0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b);
    }

    const FixedPointValue operator/(int a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, int b) {
        return FixedPointValue(a.to_double() + b);
    }

    const FixedPointValue operator+(int a, const FixedPoint& b) {
        return FixedPointValue(a + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, int b) {
        return FixedPointValue(a.to_double() - b);
    }

    const FixedPointValue operator-(int a, const FixedPoint& b) {
        return FixedPointValue(a - b.to_double());
    }

    // Binary operators FixedPoint vs unsigned int
    const FixedPointValue operator*(const FixedPoint& a, unsigned int b) {
        return FixedPointValue(a.to_double() * b);
    }

    const FixedPointValue operator*(unsigned int a, const FixedPoint& b) {
        return FixedPointValue(a * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, unsigned int b) {
        if (b == 0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b);
    }

    const FixedPointValue operator/(unsigned int a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, unsigned int b) {
        return FixedPointValue(a.to_double() + b);
    }

    const FixedPointValue operator+(unsigned int a, const FixedPoint& b) {
        return FixedPointValue(a + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, unsigned int b) {
        return FixedPointValue(a.to_double() - b);
    }

    const FixedPointValue operator-(unsigned int a, const FixedPoint& b) {
        return FixedPointValue(a - b.to_double());
    }

    // Binary operators FixedPoint vs long
    const FixedPointValue operator*(const FixedPoint& a, long b) {
        return FixedPointValue(a.to_double() * b);
    }

    const FixedPointValue operator*(long a, const FixedPoint& b) {
        return FixedPointValue(a * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, long b) {
        if (b == 0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b);
    }

    const FixedPointValue operator/(long a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, long b) {
        return FixedPointValue(a.to_double() + b);
    }

    const FixedPointValue operator+(long a, const FixedPoint& b) {
        return FixedPointValue(a + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, long b) {
        return FixedPointValue(a.to_double() - b);
    }

    const FixedPointValue operator-(long a, const FixedPoint& b) {
        return FixedPointValue(a - b.to_double());
    }

    // Binary operators FixedPoint vs unsigned long
    const FixedPointValue operator*(const FixedPoint& a, unsigned long b) {
        return FixedPointValue(a.to_double() * b);
    }

    const FixedPointValue operator*(unsigned long a, const FixedPoint& b) {
        return FixedPointValue(a * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, unsigned long b) {
        if (b == 0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b);
    }

    const FixedPointValue operator/(unsigned long a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, unsigned long b) {
        return FixedPointValue(a.to_double() + b);
    }

    const FixedPointValue operator+(unsigned long a, const FixedPoint& b) {
        return FixedPointValue(a + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, unsigned long b) {
        return FixedPointValue(a.to_double() - b);
    }

    const FixedPointValue operator-(unsigned long a, const FixedPoint& b) {
        return FixedPointValue(a - b.to_double());
    }

    // Binary operators FixedPoint vs double
    const FixedPointValue operator*(const FixedPoint& a, double b) {
        return FixedPointValue(a.to_double() * b);
    }

    const FixedPointValue operator*(double a, const FixedPoint& b) {
        return FixedPointValue(a * b.to_double());
    }

    const FixedPointValue operator/(const FixedPoint& a, double b) {
        if (b == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a.to_double() / b);
    }

    const FixedPointValue operator/(double a, const FixedPoint& b) {
        if (b.to_double() == 0.0) return FixedPointValue(0.0);
        return FixedPointValue(a / b.to_double());
    }

    const FixedPointValue operator+(const FixedPoint& a, double b) {
        return FixedPointValue(a.to_double() + b);
    }

    const FixedPointValue operator+(double a, const FixedPoint& b) {
        return FixedPointValue(a + b.to_double());
    }

    const FixedPointValue operator-(const FixedPoint& a, double b) {
        return FixedPointValue(a.to_double() - b);
    }

    const FixedPointValue operator-(double a, const FixedPoint& b) {
        return FixedPointValue(a - b.to_double());
    }

    // Shift operators
    const FixedPointValue operator<<(const FixedPoint& a, int b) {
        return FixedPointValue(a.to_double() * (1 << b));
    }

    const FixedPointValue operator>>(const FixedPoint& a, int b) {
        return FixedPointValue(a.to_double() / (1 << b));
    }

    // Relational operators FixedPoint vs FixedPoint
    bool operator<(const FixedPoint& a, const FixedPoint& b) { return a.to_double() < b.to_double(); }
    bool operator<=(const FixedPoint& a, const FixedPoint& b) { return a.to_double() <= b.to_double(); }
    bool operator>(const FixedPoint& a, const FixedPoint& b) { return a.to_double() > b.to_double(); }
    bool operator>=(const FixedPoint& a, const FixedPoint& b) { return a.to_double() >= b.to_double(); }
    bool operator==(const FixedPoint& a, const FixedPoint& b) { return a.to_double() == b.to_double(); }
    bool operator!=(const FixedPoint& a, const FixedPoint& b) { return a.to_double() != b.to_double(); }

    // Relational operators FixedPoint vs FixedPointValue
    bool operator<(const FixedPoint& a, const FixedPointValue& b) { return a.to_double() < b.to_double(); }
    bool operator<(const FixedPointValue& a, const FixedPoint& b) { return a.to_double() < b.to_double(); }
    bool operator<=(const FixedPoint& a, const FixedPointValue& b) { return a.to_double() <= b.to_double(); }
    bool operator<=(const FixedPointValue& a, const FixedPoint& b) { return a.to_double() <= b.to_double(); }
    bool operator>(const FixedPoint& a, const FixedPointValue& b) { return a.to_double() > b.to_double(); }
    bool operator>(const FixedPointValue& a, const FixedPoint& b) { return a.to_double() > b.to_double(); }
    bool operator>=(const FixedPoint& a, const FixedPointValue& b) { return a.to_double() >= b.to_double(); }
    bool operator>=(const FixedPointValue& a, const FixedPoint& b) { return a.to_double() >= b.to_double(); }
    bool operator==(const FixedPoint& a, const FixedPointValue& b) { return a.to_double() == b.to_double(); }
    bool operator==(const FixedPointValue& a, const FixedPoint& b) { return a.to_double() == b.to_double(); }
    bool operator!=(const FixedPoint& a, const FixedPointValue& b) { return a.to_double() != b.to_double(); }
    bool operator!=(const FixedPointValue& a, const FixedPoint& b) { return a.to_double() != b.to_double(); }

    // Relational operators FixedPoint vs int
    bool operator<(const FixedPoint& a, int b) { return a.to_double() < b; }
    bool operator<(int a, const FixedPoint& b) { return a < b.to_double(); }
    bool operator<=(const FixedPoint& a, int b) { return a.to_double() <= b; }
    bool operator<=(int a, const FixedPoint& b) { return a <= b.to_double(); }
    bool operator>(const FixedPoint& a, int b) { return a.to_double() > b; }
    bool operator>(int a, const FixedPoint& b) { return a > b.to_double(); }
    bool operator>=(const FixedPoint& a, int b) { return a.to_double() >= b; }
    bool operator>=(int a, const FixedPoint& b) { return a >= b.to_double(); }
    bool operator==(const FixedPoint& a, int b) { return a.to_double() == b; }
    bool operator==(int a, const FixedPoint& b) { return a == b.to_double(); }
    bool operator!=(const FixedPoint& a, int b) { return a.to_double() != b; }
    bool operator!=(int a, const FixedPoint& b) { return a != b.to_double(); }

    // Relational operators FixedPoint vs unsigned int
    bool operator<(const FixedPoint& a, unsigned int b) { return a.to_double() < b; }
    bool operator<(unsigned int a, const FixedPoint& b) { return a < b.to_double(); }
    bool operator<=(const FixedPoint& a, unsigned int b) { return a.to_double() <= b; }
    bool operator<=(unsigned int a, const FixedPoint& b) { return a <= b.to_double(); }
    bool operator>(const FixedPoint& a, unsigned int b) { return a.to_double() > b; }
    bool operator>(unsigned int a, const FixedPoint& b) { return a > b.to_double(); }
    bool operator>=(const FixedPoint& a, unsigned int b) { return a.to_double() >= b; }
    bool operator>=(unsigned int a, const FixedPoint& b) { return a >= b.to_double(); }
    bool operator==(const FixedPoint& a, unsigned int b) { return a.to_double() == b; }
    bool operator==(unsigned int a, const FixedPoint& b) { return a == b.to_double(); }
    bool operator!=(const FixedPoint& a, unsigned int b) { return a.to_double() != b; }
    bool operator!=(unsigned int a, const FixedPoint& b) { return a != b.to_double(); }

    // Relational operators FixedPoint vs long
    bool operator<(const FixedPoint& a, long b) { return a.to_double() < b; }
    bool operator<(long a, const FixedPoint& b) { return a < b.to_double(); }
    bool operator<=(const FixedPoint& a, long b) { return a.to_double() <= b; }
    bool operator<=(long a, const FixedPoint& b) { return a <= b.to_double(); }
    bool operator>(const FixedPoint& a, long b) { return a.to_double() > b; }
    bool operator>(long a, const FixedPoint& b) { return a > b.to_double(); }
    bool operator>=(const FixedPoint& a, long b) { return a.to_double() >= b; }
    bool operator>=(long a, const FixedPoint& b) { return a >= b.to_double(); }
    bool operator==(const FixedPoint& a, long b) { return a.to_double() == b; }
    bool operator==(long a, const FixedPoint& b) { return a == b.to_double(); }
    bool operator!=(const FixedPoint& a, long b) { return a.to_double() != b; }
    bool operator!=(long a, const FixedPoint& b) { return a != b.to_double(); }

    // Relational operators FixedPoint vs unsigned long
    bool operator<(const FixedPoint& a, unsigned long b) { return a.to_double() < b; }
    bool operator<(unsigned long a, const FixedPoint& b) { return a < b.to_double(); }
    bool operator<=(const FixedPoint& a, unsigned long b) { return a.to_double() <= b; }
    bool operator<=(unsigned long a, const FixedPoint& b) { return a <= b.to_double(); }
    bool operator>(const FixedPoint& a, unsigned long b) { return a.to_double() > b; }
    bool operator>(unsigned long a, const FixedPoint& b) { return a > b.to_double(); }
    bool operator>=(const FixedPoint& a, unsigned long b) { return a.to_double() >= b; }
    bool operator>=(unsigned long a, const FixedPoint& b) { return a >= b.to_double(); }
    bool operator==(const FixedPoint& a, unsigned long b) { return a.to_double() == b; }
    bool operator==(unsigned long a, const FixedPoint& b) { return a == b.to_double(); }
    bool operator!=(const FixedPoint& a, unsigned long b) { return a.to_double() != b; }
    bool operator!=(unsigned long a, const FixedPoint& b) { return a != b.to_double(); }

    // Relational operators FixedPoint vs double
    bool operator<(const FixedPoint& a, double b) { return a.to_double() < b; }
    bool operator<(double a, const FixedPoint& b) { return a < b.to_double(); }
    bool operator<=(const FixedPoint& a, double b) { return a.to_double() <= b; }
    bool operator<=(double a, const FixedPoint& b) { return a <= b.to_double(); }
    bool operator>(const FixedPoint& a, double b) { return a.to_double() > b; }
    bool operator>(double a, const FixedPoint& b) { return a > b.to_double(); }
    bool operator>=(const FixedPoint& a, double b) { return a.to_double() >= b; }
    bool operator>=(double a, const FixedPoint& b) { return a >= b.to_double(); }
    bool operator==(const FixedPoint& a, double b) { return a.to_double() == b; }
    bool operator==(double a, const FixedPoint& b) { return a == b.to_double(); }
    bool operator!=(const FixedPoint& a, double b) { return a.to_double() != b; }
    bool operator!=(double a, const FixedPoint& b) { return a != b.to_double(); }

    // Bitwise operators
    const FixedPoint operator&(const FixedPoint& a, const FixedPoint& b) {
        FixedPoint result(a);
        result.m_rep->value = static_cast<long long>(a.to_double()) & static_cast<long long>(b.to_double());
        return result;
    }

    const FixedPoint operator|(const FixedPoint& a, const FixedPoint& b) {
        FixedPoint result(a);
        result.m_rep->value = static_cast<long long>(a.to_double()) | static_cast<long long>(b.to_double());
        return result;
    }

    const FixedPoint operator^(const FixedPoint& a, const FixedPoint& b) {
        FixedPoint result(a);
        result.m_rep->value = static_cast<long long>(a.to_double()) ^ static_cast<long long>(b.to_double());
        return result;
    }

    // Assignment operators
    FixedPoint& FixedPoint::operator=(int a) { m_rep->value = a; cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(int a) { m_rep->value *= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(int a) { if (a != 0) m_rep->value /= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(int a) { m_rep->value += a; cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(int a) { m_rep->value -= a; cast(); return *this; }

    FixedPoint& FixedPoint::operator=(unsigned int a) { m_rep->value = a; cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(unsigned int a) { m_rep->value *= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(unsigned int a) { if (a != 0) m_rep->value /= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(unsigned int a) { m_rep->value += a; cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(unsigned int a) { m_rep->value -= a; cast(); return *this; }

    FixedPoint& FixedPoint::operator=(long a) { m_rep->value = a; cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(long a) { m_rep->value *= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(long a) { if (a != 0) m_rep->value /= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(long a) { m_rep->value += a; cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(long a) { m_rep->value -= a; cast(); return *this; }

    FixedPoint& FixedPoint::operator=(unsigned long a) { m_rep->value = a; cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(unsigned long a) { m_rep->value *= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(unsigned long a) { if (a != 0) m_rep->value /= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(unsigned long a) { m_rep->value += a; cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(unsigned long a) { m_rep->value -= a; cast(); return *this; }

    FixedPoint& FixedPoint::operator=(double a) { m_rep->value = a; cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(double a) { m_rep->value *= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(double a) { if (a != 0.0) m_rep->value /= a; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(double a) { m_rep->value += a; cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(double a) { m_rep->value -= a; cast(); return *this; }

    FixedPoint& FixedPoint::operator=(const FixedPointValue& a) { m_rep->value = a.to_double(); cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(const FixedPointValue& a) { m_rep->value *= a.to_double(); cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(const FixedPointValue& a) { double d = a.to_double(); if (d != 0.0) m_rep->value /= d; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(const FixedPointValue& a) { m_rep->value += a.to_double(); cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(const FixedPointValue& a) { m_rep->value -= a.to_double(); cast(); return *this; }

    FixedPoint& FixedPoint::operator=(const FixedPoint& a) { m_rep->value = a.m_rep->value; m_parameters = a.m_parameters; cast(); return *this; }
    FixedPoint& FixedPoint::operator*=(const FixedPoint& a) { m_rep->value *= a.m_rep->value; cast(); return *this; }
    FixedPoint& FixedPoint::operator/=(const FixedPoint& a) { if (a.m_rep->value != 0.0) m_rep->value /= a.m_rep->value; cast(); return *this; }
    FixedPoint& FixedPoint::operator+=(const FixedPoint& a) { m_rep->value += a.m_rep->value; cast(); return *this; }
    FixedPoint& FixedPoint::operator-=(const FixedPoint& a) { m_rep->value -= a.m_rep->value; cast(); return *this; }

    FixedPoint& FixedPoint::operator<<=(int a) { m_rep->value *= (1 << a); cast(); return *this; }
    FixedPoint& FixedPoint::operator>>=(int a) { m_rep->value /= (1 << a); cast(); return *this; }

    // Auto-increment and auto-decrement
    const FixedPointValue FixedPoint::operator++(int) {
        FixedPointValue temp(m_rep->value);
        m_rep->value++;
        cast();
        return temp;
    }

    const FixedPointValue FixedPoint::operator--(int) {
        FixedPointValue temp(m_rep->value);
        m_rep->value--;
        cast();
        return temp;
    }

    FixedPoint& FixedPoint::operator++() {
        m_rep->value++;
        cast();
        return *this;
    }

    FixedPoint& FixedPoint::operator--() {
        m_rep->value--;
        cast();
        return *this;
    }

    // Bit selection
    const FixedPointBitRef FixedPoint::operator[](int i) const {
        return FixedPointBitRef(const_cast<FixedPoint&>(*this), i);
    }

    FixedPointBitRef FixedPoint::operator[](int i) {
        return FixedPointBitRef(*this, i);
    }

    const FixedPointBitRef FixedPoint::bit(int i) const {
        return FixedPointBitRef(const_cast<FixedPoint&>(*this), i);
    }

    FixedPointBitRef FixedPoint::bit(int i) {
        return FixedPointBitRef(*this, i);
    }

    // Explicit conversion methods
    short FixedPoint::to_short() const { return static_cast<short>(m_rep->value); }
    unsigned short FixedPoint::to_ushort() const { return static_cast<unsigned short>(m_rep->value); }
    int FixedPoint::to_int() const { return static_cast<int>(m_rep->value); }
    unsigned int FixedPoint::to_uint() const { return static_cast<unsigned int>(m_rep->value); }
    long FixedPoint::to_long() const { return static_cast<long>(m_rep->value); }
    unsigned long FixedPoint::to_ulong() const { return static_cast<unsigned long>(m_rep->value); }
    float FixedPoint::to_float() const { return static_cast<float>(m_rep->value); }
    double FixedPoint::to_double() const { return m_rep->value; }

    const std::string FixedPoint::to_dec() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(10) << m_rep->value;
        return oss.str();
    }

    const std::string FixedPoint::to_bin() const {
        long long intVal = static_cast<long long>(m_rep->value);
        std::string result;
        if (intVal == 0) return "0";
        bool negative = intVal < 0;
        if (negative) intVal = -intVal;
        while (intVal > 0) {
            result = (intVal % 2 ? "1" : "0") + result;
            intVal /= 2;
        }
        if (negative) result = "-" + result;
        return result;
    }

    const std::string FixedPoint::to_oct() const {
        long long intVal = static_cast<long long>(m_rep->value);
        std::ostringstream oss;
        oss << std::oct << intVal;
        return oss.str();
    }

    const std::string FixedPoint::to_hex() const {
        long long intVal = static_cast<long long>(m_rep->value);
        std::ostringstream oss;
        oss << std::hex << intVal;
        return oss.str();
    }

    // Query methods
    bool FixedPoint::is_neg() const { return m_rep->value < 0; }
    bool FixedPoint::is_zero() const { return m_rep->value == 0.0; }
    bool FixedPoint::is_normal() const { return std::isnormal(m_rep->value) || m_rep->value == 0.0; }
    bool FixedPoint::quantization_flag() const { return m_quantizationFlag; }
    bool FixedPoint::overflow_flag() const { return m_overflowFlag; }

    int FixedPoint::wl() const { return m_parameters.wl(); }
    int FixedPoint::iwl() const { return m_parameters.iwl(); }
    FixedPointEnums::QuantizationMode FixedPoint::q_mode() const { return m_parameters.q_mode(); }
    FixedPointEnums::OverflowMode FixedPoint::o_mode() const { return m_parameters.o_mode(); }
    FixedPointEnums::Sign FixedPoint::sign() const { return m_parameters.sign(); }
    int FixedPoint::saturationBits() const { return m_parameters.saturationBits(); }
    const FixedPointParameters& FixedPoint::getParameters() const { return m_parameters; }

    bool FixedPoint::Zero(FixedPoint* pReference) {
        m_rep->value = 0.0;
        if (pReference) {
            m_parameters = pReference->m_parameters;
        }
        return true;
    }

    bool FixedPoint::get_bit(int i) const {
        long long intVal = static_cast<long long>(m_rep->value);
        return (intVal >> i) & 1;
    }

    bool FixedPoint::set_bit(int i, bool val) {
        long long intVal = static_cast<long long>(m_rep->value);
        if (val) {
            intVal |= (1LL << i);
        } else {
            intVal &= ~(1LL << i);
        }
        m_rep->value = static_cast<double>(intVal);
        return true;
    }

} // namespace SystemVueModelBuilder
