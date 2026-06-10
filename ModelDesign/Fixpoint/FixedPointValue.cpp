// Copyright 2011 - 2014 Keysight Technologies, Inc
#include "FixedPointValue.h"
#include "FixedPoint.h"
#include "FixedPointParameters.h"
#include "FixedPointRep.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace SystemVueModelBuilder
{
    // Constructors
    FixedPointValue::FixedPointValue()
        : m_rep(new FixedPointRep())
    {
    }

    FixedPointValue::FixedPointValue(int val)
        : m_rep(new FixedPointRep(static_cast<double>(val), 32, 32, FixedPointEnums::TWOS_COMPLEMENT))
    {
    }

    FixedPointValue::FixedPointValue(unsigned int val)
        : m_rep(new FixedPointRep(static_cast<double>(val), 32, 32, FixedPointEnums::UNSIGNED))
    {
    }

    FixedPointValue::FixedPointValue(long val)
        : m_rep(new FixedPointRep(static_cast<double>(val), 64, 64, FixedPointEnums::TWOS_COMPLEMENT))
    {
    }

    FixedPointValue::FixedPointValue(unsigned long val)
        : m_rep(new FixedPointRep(static_cast<double>(val), 64, 64, FixedPointEnums::UNSIGNED))
    {
    }

    FixedPointValue::FixedPointValue(double val)
        : m_rep(new FixedPointRep(val, 64, 32, FixedPointEnums::TWOS_COMPLEMENT))
    {
    }

    FixedPointValue::FixedPointValue(const FixedPointValue& other)
        : m_rep(new FixedPointRep(*other.m_rep))
    {
    }

    FixedPointValue::FixedPointValue(const FixedPoint& other)
        : m_rep(new FixedPointRep())
    {
        m_rep->value = other.to_double();
        m_rep->wl = other.wl();
        m_rep->iwl = other.iwl();
        m_rep->sign = other.sign();
    }

    FixedPointValue::FixedPointValue(FixedPointRep* rep)
        : m_rep(rep)
    {
    }

    FixedPointValue::~FixedPointValue()
    {
        delete m_rep;
    }

    const FixedPointRep* FixedPointValue::get_rep() const { return m_rep; }
    void FixedPointValue::set_rep(FixedPointRep* rep) { delete m_rep; m_rep = rep; }

    // Unary operators
    const FixedPointValue FixedPointValue::operator-() const {
        FixedPointRep* rep = new FixedPointRep(-m_rep->value, m_rep->wl, m_rep->iwl, m_rep->sign);
        return FixedPointValue(rep);
    }

    const FixedPointValue& FixedPointValue::operator+() const {
        return *this;
    }

    void neg(FixedPointValue& result, const FixedPointValue& val) {
        result.set_rep(new FixedPointRep(-val.get_rep()->value, val.get_rep()->wl, val.get_rep()->iwl, val.get_rep()->sign));
    }

    // Binary operators FixedPointValue vs FixedPointValue
    const FixedPointValue operator*(const FixedPointValue& a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * b.get_rep()->value,
            std::max(a.get_rep()->wl, b.get_rep()->wl), a.get_rep()->iwl + b.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator+(const FixedPointValue& a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value + b.get_rep()->value,
            std::max(a.get_rep()->wl, b.get_rep()->wl) + 1, std::max(a.get_rep()->iwl, b.get_rep()->iwl) + 1, a.get_rep()->sign));
    }

    const FixedPointValue operator-(const FixedPointValue& a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value - b.get_rep()->value,
            std::max(a.get_rep()->wl, b.get_rep()->wl) + 1, std::max(a.get_rep()->iwl, b.get_rep()->iwl) + 1, a.get_rep()->sign));
    }

    const FixedPointValue operator/(const FixedPointValue& a, const FixedPointValue& b) {
        if (b.get_rep()->value == 0.0) {
            return FixedPointValue(new FixedPointRep(0.0, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
        }
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / b.get_rep()->value,
            a.get_rep()->wl + b.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    // Binary operators FixedPointValue vs int
    const FixedPointValue operator*(const FixedPointValue& a, int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator*(int a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a * b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    const FixedPointValue operator+(const FixedPointValue& a, int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value + b, a.get_rep()->wl + 32, a.get_rep()->iwl + 32, a.get_rep()->sign));
    }

    const FixedPointValue operator+(int a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a + b.get_rep()->value, b.get_rep()->wl + 32, b.get_rep()->iwl + 32, b.get_rep()->sign));
    }

    const FixedPointValue operator-(const FixedPointValue& a, int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value - b, a.get_rep()->wl + 32, a.get_rep()->iwl + 32, a.get_rep()->sign));
    }

    const FixedPointValue operator-(int a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a - b.get_rep()->value, b.get_rep()->wl + 32, b.get_rep()->iwl + 32, b.get_rep()->sign));
    }

    const FixedPointValue operator/(const FixedPointValue& a, int b) {
        if (b == 0) return FixedPointValue(new FixedPointRep(0.0, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator/(int a, const FixedPointValue& b) {
        if (b.get_rep()->value == 0.0) return FixedPointValue(new FixedPointRep(0.0, 32, 32, FixedPointEnums::TWOS_COMPLEMENT));
        return FixedPointValue(new FixedPointRep(a / b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    // Binary operators FixedPointValue vs unsigned int
    const FixedPointValue operator*(const FixedPointValue& a, unsigned int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator*(unsigned int a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a * b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    const FixedPointValue operator+(const FixedPointValue& a, unsigned int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value + b, a.get_rep()->wl + 32, a.get_rep()->iwl + 32, a.get_rep()->sign));
    }

    const FixedPointValue operator+(unsigned int a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a + b.get_rep()->value, b.get_rep()->wl + 32, b.get_rep()->iwl + 32, b.get_rep()->sign));
    }

    const FixedPointValue operator-(const FixedPointValue& a, unsigned int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value - b, a.get_rep()->wl + 32, a.get_rep()->iwl + 32, a.get_rep()->sign));
    }

    const FixedPointValue operator-(unsigned int a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a - b.get_rep()->value, b.get_rep()->wl + 32, b.get_rep()->iwl + 32, b.get_rep()->sign));
    }

    const FixedPointValue operator/(const FixedPointValue& a, unsigned int b) {
        if (b == 0) return FixedPointValue(new FixedPointRep(0.0, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator/(unsigned int a, const FixedPointValue& b) {
        if (b.get_rep()->value == 0.0) return FixedPointValue(new FixedPointRep(0.0, 32, 32, FixedPointEnums::UNSIGNED));
        return FixedPointValue(new FixedPointRep(a / b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    // Binary operators FixedPointValue vs long
    const FixedPointValue operator*(const FixedPointValue& a, long b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator*(long a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a * b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    const FixedPointValue operator+(const FixedPointValue& a, long b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value + b, a.get_rep()->wl + 64, a.get_rep()->iwl + 64, a.get_rep()->sign));
    }

    const FixedPointValue operator+(long a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a + b.get_rep()->value, b.get_rep()->wl + 64, b.get_rep()->iwl + 64, b.get_rep()->sign));
    }

    const FixedPointValue operator-(const FixedPointValue& a, long b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value - b, a.get_rep()->wl + 64, a.get_rep()->iwl + 64, a.get_rep()->sign));
    }

    const FixedPointValue operator-(long a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a - b.get_rep()->value, b.get_rep()->wl + 64, b.get_rep()->iwl + 64, b.get_rep()->sign));
    }

    const FixedPointValue operator/(const FixedPointValue& a, long b) {
        if (b == 0) return FixedPointValue(new FixedPointRep(0.0, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator/(long a, const FixedPointValue& b) {
        if (b.get_rep()->value == 0.0) return FixedPointValue(new FixedPointRep(0.0, 64, 64, FixedPointEnums::TWOS_COMPLEMENT));
        return FixedPointValue(new FixedPointRep(a / b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    // Binary operators FixedPointValue vs unsigned long
    const FixedPointValue operator*(const FixedPointValue& a, unsigned long b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator*(unsigned long a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a * b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    const FixedPointValue operator+(const FixedPointValue& a, unsigned long b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value + b, a.get_rep()->wl + 64, a.get_rep()->iwl + 64, a.get_rep()->sign));
    }

    const FixedPointValue operator+(unsigned long a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a + b.get_rep()->value, b.get_rep()->wl + 64, b.get_rep()->iwl + 64, b.get_rep()->sign));
    }

    const FixedPointValue operator-(const FixedPointValue& a, unsigned long b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value - b, a.get_rep()->wl + 64, a.get_rep()->iwl + 64, a.get_rep()->sign));
    }

    const FixedPointValue operator-(unsigned long a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a - b.get_rep()->value, b.get_rep()->wl + 64, b.get_rep()->iwl + 64, b.get_rep()->sign));
    }

    const FixedPointValue operator/(const FixedPointValue& a, unsigned long b) {
        if (b == 0) return FixedPointValue(new FixedPointRep(0.0, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator/(unsigned long a, const FixedPointValue& b) {
        if (b.get_rep()->value == 0.0) return FixedPointValue(new FixedPointRep(0.0, 64, 64, FixedPointEnums::UNSIGNED));
        return FixedPointValue(new FixedPointRep(a / b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    // Binary operators FixedPointValue vs double
    const FixedPointValue operator*(const FixedPointValue& a, double b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator*(double a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a * b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    const FixedPointValue operator+(const FixedPointValue& a, double b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value + b, a.get_rep()->wl + 64, a.get_rep()->iwl + 32, a.get_rep()->sign));
    }

    const FixedPointValue operator+(double a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a + b.get_rep()->value, b.get_rep()->wl + 64, b.get_rep()->iwl + 32, b.get_rep()->sign));
    }

    const FixedPointValue operator-(const FixedPointValue& a, double b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value - b, a.get_rep()->wl + 64, a.get_rep()->iwl + 32, a.get_rep()->sign));
    }

    const FixedPointValue operator-(double a, const FixedPointValue& b) {
        return FixedPointValue(new FixedPointRep(a - b.get_rep()->value, b.get_rep()->wl + 64, b.get_rep()->iwl + 32, b.get_rep()->sign));
    }

    const FixedPointValue operator/(const FixedPointValue& a, double b) {
        if (b == 0.0) return FixedPointValue(new FixedPointRep(0.0, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / b, a.get_rep()->wl, a.get_rep()->iwl, a.get_rep()->sign));
    }

    const FixedPointValue operator/(double a, const FixedPointValue& b) {
        if (b.get_rep()->value == 0.0) return FixedPointValue(new FixedPointRep(0.0, 64, 32, FixedPointEnums::TWOS_COMPLEMENT));
        return FixedPointValue(new FixedPointRep(a / b.get_rep()->value, b.get_rep()->wl, b.get_rep()->iwl, b.get_rep()->sign));
    }

    // Shift operators
    const FixedPointValue operator<<(const FixedPointValue& a, int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value * (1 << b), a.get_rep()->wl + b, a.get_rep()->iwl + b, a.get_rep()->sign));
    }

    const FixedPointValue operator>>(const FixedPointValue& a, int b) {
        return FixedPointValue(new FixedPointRep(a.get_rep()->value / (1 << b), a.get_rep()->wl - b, a.get_rep()->iwl - b, a.get_rep()->sign));
    }

    void lshift(FixedPointValue& result, const FixedPointValue& val, int shift) {
        result.set_rep(new FixedPointRep(val.get_rep()->value * (1 << shift), val.get_rep()->wl + shift, val.get_rep()->iwl + shift, val.get_rep()->sign));
    }

    void rshift(FixedPointValue& result, const FixedPointValue& val, int shift) {
        result.set_rep(new FixedPointRep(val.get_rep()->value / (1 << shift), val.get_rep()->wl - shift, val.get_rep()->iwl - shift, val.get_rep()->sign));
    }

    // Relational operators FixedPointValue vs FixedPointValue
    bool operator<(const FixedPointValue& a, const FixedPointValue& b) { return a.get_rep()->value < b.get_rep()->value; }
    bool operator<=(const FixedPointValue& a, const FixedPointValue& b) { return a.get_rep()->value <= b.get_rep()->value; }
    bool operator>(const FixedPointValue& a, const FixedPointValue& b) { return a.get_rep()->value > b.get_rep()->value; }
    bool operator>=(const FixedPointValue& a, const FixedPointValue& b) { return a.get_rep()->value >= b.get_rep()->value; }
    bool operator==(const FixedPointValue& a, const FixedPointValue& b) { return a.get_rep()->value == b.get_rep()->value; }
    bool operator!=(const FixedPointValue& a, const FixedPointValue& b) { return a.get_rep()->value != b.get_rep()->value; }

    // Relational operators FixedPointValue vs int
    bool operator<(const FixedPointValue& a, int b) { return a.get_rep()->value < b; }
    bool operator<(int a, const FixedPointValue& b) { return a < b.get_rep()->value; }
    bool operator<=(const FixedPointValue& a, int b) { return a.get_rep()->value <= b; }
    bool operator<=(int a, const FixedPointValue& b) { return a <= b.get_rep()->value; }
    bool operator>(const FixedPointValue& a, int b) { return a.get_rep()->value > b; }
    bool operator>(int a, const FixedPointValue& b) { return a > b.get_rep()->value; }
    bool operator>=(const FixedPointValue& a, int b) { return a.get_rep()->value >= b; }
    bool operator>=(int a, const FixedPointValue& b) { return a >= b.get_rep()->value; }
    bool operator==(const FixedPointValue& a, int b) { return a.get_rep()->value == b; }
    bool operator==(int a, const FixedPointValue& b) { return a == b.get_rep()->value; }
    bool operator!=(const FixedPointValue& a, int b) { return a.get_rep()->value != b; }
    bool operator!=(int a, const FixedPointValue& b) { return a != b.get_rep()->value; }

    // Relational operators FixedPointValue vs unsigned int
    bool operator<(const FixedPointValue& a, unsigned int b) { return a.get_rep()->value < b; }
    bool operator<(unsigned int a, const FixedPointValue& b) { return a < b.get_rep()->value; }
    bool operator<=(const FixedPointValue& a, unsigned int b) { return a.get_rep()->value <= b; }
    bool operator<=(unsigned int a, const FixedPointValue& b) { return a <= b.get_rep()->value; }
    bool operator>(const FixedPointValue& a, unsigned int b) { return a.get_rep()->value > b; }
    bool operator>(unsigned int a, const FixedPointValue& b) { return a > b.get_rep()->value; }
    bool operator>=(const FixedPointValue& a, unsigned int b) { return a.get_rep()->value >= b; }
    bool operator>=(unsigned int a, const FixedPointValue& b) { return a >= b.get_rep()->value; }
    bool operator==(const FixedPointValue& a, unsigned int b) { return a.get_rep()->value == b; }
    bool operator==(unsigned int a, const FixedPointValue& b) { return a == b.get_rep()->value; }
    bool operator!=(const FixedPointValue& a, unsigned int b) { return a.get_rep()->value != b; }
    bool operator!=(unsigned int a, const FixedPointValue& b) { return a != b.get_rep()->value; }

    // Relational operators FixedPointValue vs long
    bool operator<(const FixedPointValue& a, long b) { return a.get_rep()->value < b; }
    bool operator<(long a, const FixedPointValue& b) { return a < b.get_rep()->value; }
    bool operator<=(const FixedPointValue& a, long b) { return a.get_rep()->value <= b; }
    bool operator<=(long a, const FixedPointValue& b) { return a <= b.get_rep()->value; }
    bool operator>(const FixedPointValue& a, long b) { return a.get_rep()->value > b; }
    bool operator>(long a, const FixedPointValue& b) { return a > b.get_rep()->value; }
    bool operator>=(const FixedPointValue& a, long b) { return a.get_rep()->value >= b; }
    bool operator>=(long a, const FixedPointValue& b) { return a >= b.get_rep()->value; }
    bool operator==(const FixedPointValue& a, long b) { return a.get_rep()->value == b; }
    bool operator==(long a, const FixedPointValue& b) { return a == b.get_rep()->value; }
    bool operator!=(const FixedPointValue& a, long b) { return a.get_rep()->value != b; }
    bool operator!=(long a, const FixedPointValue& b) { return a != b.get_rep()->value; }

    // Relational operators FixedPointValue vs unsigned long
    bool operator<(const FixedPointValue& a, unsigned long b) { return a.get_rep()->value < b; }
    bool operator<(unsigned long a, const FixedPointValue& b) { return a < b.get_rep()->value; }
    bool operator<=(const FixedPointValue& a, unsigned long b) { return a.get_rep()->value <= b; }
    bool operator<=(unsigned long a, const FixedPointValue& b) { return a <= b.get_rep()->value; }
    bool operator>(const FixedPointValue& a, unsigned long b) { return a.get_rep()->value > b; }
    bool operator>(unsigned long a, const FixedPointValue& b) { return a > b.get_rep()->value; }
    bool operator>=(const FixedPointValue& a, unsigned long b) { return a.get_rep()->value >= b; }
    bool operator>=(unsigned long a, const FixedPointValue& b) { return a >= b.get_rep()->value; }
    bool operator==(const FixedPointValue& a, unsigned long b) { return a.get_rep()->value == b; }
    bool operator==(unsigned long a, const FixedPointValue& b) { return a == b.get_rep()->value; }
    bool operator!=(const FixedPointValue& a, unsigned long b) { return a.get_rep()->value != b; }
    bool operator!=(unsigned long a, const FixedPointValue& b) { return a != b.get_rep()->value; }

    // Relational operators FixedPointValue vs double
    bool operator<(const FixedPointValue& a, double b) { return a.get_rep()->value < b; }
    bool operator<(double a, const FixedPointValue& b) { return a < b.get_rep()->value; }
    bool operator<=(const FixedPointValue& a, double b) { return a.get_rep()->value <= b; }
    bool operator<=(double a, const FixedPointValue& b) { return a <= b.get_rep()->value; }
    bool operator>(const FixedPointValue& a, double b) { return a.get_rep()->value > b; }
    bool operator>(double a, const FixedPointValue& b) { return a > b.get_rep()->value; }
    bool operator>=(const FixedPointValue& a, double b) { return a.get_rep()->value >= b; }
    bool operator>=(double a, const FixedPointValue& b) { return a >= b.get_rep()->value; }
    bool operator==(const FixedPointValue& a, double b) { return a.get_rep()->value == b; }
    bool operator==(double a, const FixedPointValue& b) { return a == b.get_rep()->value; }
    bool operator!=(const FixedPointValue& a, double b) { return a.get_rep()->value != b; }
    bool operator!=(double a, const FixedPointValue& b) { return a != b.get_rep()->value; }

    // Assignment operators
    FixedPointValue& FixedPointValue::operator=(int a) { m_rep->value = a; return *this; }
    FixedPointValue& FixedPointValue::operator*=(int a) { m_rep->value *= a; return *this; }
    FixedPointValue& FixedPointValue::operator/=(int a) { if (a != 0) m_rep->value /= a; return *this; }
    FixedPointValue& FixedPointValue::operator+=(int a) { m_rep->value += a; return *this; }
    FixedPointValue& FixedPointValue::operator-=(int a) { m_rep->value -= a; return *this; }

    FixedPointValue& FixedPointValue::operator=(unsigned int a) { m_rep->value = a; return *this; }
    FixedPointValue& FixedPointValue::operator*=(unsigned int a) { m_rep->value *= a; return *this; }
    FixedPointValue& FixedPointValue::operator/=(unsigned int a) { if (a != 0) m_rep->value /= a; return *this; }
    FixedPointValue& FixedPointValue::operator+=(unsigned int a) { m_rep->value += a; return *this; }
    FixedPointValue& FixedPointValue::operator-=(unsigned int a) { m_rep->value -= a; return *this; }

    FixedPointValue& FixedPointValue::operator=(long a) { m_rep->value = a; return *this; }
    FixedPointValue& FixedPointValue::operator*=(long a) { m_rep->value *= a; return *this; }
    FixedPointValue& FixedPointValue::operator/=(long a) { if (a != 0) m_rep->value /= a; return *this; }
    FixedPointValue& FixedPointValue::operator+=(long a) { m_rep->value += a; return *this; }
    FixedPointValue& FixedPointValue::operator-=(long a) { m_rep->value -= a; return *this; }

    FixedPointValue& FixedPointValue::operator=(unsigned long a) { m_rep->value = a; return *this; }
    FixedPointValue& FixedPointValue::operator*=(unsigned long a) { m_rep->value *= a; return *this; }
    FixedPointValue& FixedPointValue::operator/=(unsigned long a) { if (a != 0) m_rep->value /= a; return *this; }
    FixedPointValue& FixedPointValue::operator+=(unsigned long a) { m_rep->value += a; return *this; }
    FixedPointValue& FixedPointValue::operator-=(unsigned long a) { m_rep->value -= a; return *this; }

    FixedPointValue& FixedPointValue::operator=(double a) { m_rep->value = a; return *this; }
    FixedPointValue& FixedPointValue::operator*=(double a) { m_rep->value *= a; return *this; }
    FixedPointValue& FixedPointValue::operator/=(double a) { if (a != 0.0) m_rep->value /= a; return *this; }
    FixedPointValue& FixedPointValue::operator+=(double a) { m_rep->value += a; return *this; }
    FixedPointValue& FixedPointValue::operator-=(double a) { m_rep->value -= a; return *this; }

    FixedPointValue& FixedPointValue::operator=(const FixedPointValue& a) { m_rep->value = a.m_rep->value; return *this; }
    FixedPointValue& FixedPointValue::operator*=(const FixedPointValue& a) { m_rep->value *= a.m_rep->value; return *this; }
    FixedPointValue& FixedPointValue::operator/=(const FixedPointValue& a) { if (a.m_rep->value != 0.0) m_rep->value /= a.m_rep->value; return *this; }
    FixedPointValue& FixedPointValue::operator+=(const FixedPointValue& a) { m_rep->value += a.m_rep->value; return *this; }
    FixedPointValue& FixedPointValue::operator-=(const FixedPointValue& a) { m_rep->value -= a.m_rep->value; return *this; }

    FixedPointValue& FixedPointValue::operator=(const FixedPoint& a) { m_rep->value = a.to_double(); return *this; }
    FixedPointValue& FixedPointValue::operator*=(const FixedPoint& a) { m_rep->value *= a.to_double(); return *this; }
    FixedPointValue& FixedPointValue::operator/=(const FixedPoint& a) { double d = a.to_double(); if (d != 0.0) m_rep->value /= d; return *this; }
    FixedPointValue& FixedPointValue::operator+=(const FixedPoint& a) { m_rep->value += a.to_double(); return *this; }
    FixedPointValue& FixedPointValue::operator-=(const FixedPoint& a) { m_rep->value -= a.to_double(); return *this; }

    FixedPointValue& FixedPointValue::operator<<=(int a) { m_rep->value *= (1 << a); return *this; }
    FixedPointValue& FixedPointValue::operator>>=(int a) { m_rep->value /= (1 << a); return *this; }

    // Auto-increment and auto-decrement
    const FixedPointValue FixedPointValue::operator++(int) {
        FixedPointValue temp(*this);
        m_rep->value++;
        return temp;
    }

    const FixedPointValue FixedPointValue::operator--(int) {
        FixedPointValue temp(*this);
        m_rep->value--;
        return temp;
    }

    FixedPointValue& FixedPointValue::operator++() {
        m_rep->value++;
        return *this;
    }

    FixedPointValue& FixedPointValue::operator--() {
        m_rep->value--;
        return *this;
    }

    // Explicit conversion methods
    short FixedPointValue::to_short() const { return static_cast<short>(m_rep->value); }
    unsigned short FixedPointValue::to_ushort() const { return static_cast<unsigned short>(m_rep->value); }
    int FixedPointValue::to_int() const { return static_cast<int>(m_rep->value); }
    unsigned int FixedPointValue::to_uint() const { return static_cast<unsigned int>(m_rep->value); }
    long FixedPointValue::to_long() const { return static_cast<long>(m_rep->value); }
    unsigned long FixedPointValue::to_ulong() const { return static_cast<unsigned long>(m_rep->value); }
    float FixedPointValue::to_float() const { return static_cast<float>(m_rep->value); }
    double FixedPointValue::to_double() const { return m_rep->value; }

    const std::string FixedPointValue::to_dec() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(10) << m_rep->value;
        return oss.str();
    }

    const std::string FixedPointValue::to_bin() const {
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

    const std::string FixedPointValue::to_oct() const {
        long long intVal = static_cast<long long>(m_rep->value);
        std::ostringstream oss;
        oss << std::oct << intVal;
        return oss.str();
    }

    const std::string FixedPointValue::to_hex() const {
        long long intVal = static_cast<long long>(m_rep->value);
        std::ostringstream oss;
        oss << std::hex << intVal;
        return oss.str();
    }

    // Query value
    bool FixedPointValue::is_neg() const { return m_rep->value < 0; }
    bool FixedPointValue::is_zero() const { return m_rep->value == 0.0; }
    bool FixedPointValue::is_nan() const { return std::isnan(m_rep->value); }
    bool FixedPointValue::is_inf() const { return std::isinf(m_rep->value); }
    bool FixedPointValue::is_normal() const { return std::isnormal(m_rep->value) || m_rep->value == 0.0; }
    bool FixedPointValue::rounding_flag() const { return false; }

    bool FixedPointValue::get_bit(int i) const {
        long long intVal = static_cast<long long>(m_rep->value);
        return (intVal >> i) & 1;
    }

    void FixedPointValue::get_type(int& wl, int& iwl, FixedPointEnums::Sign& sign) const {
        wl = m_rep->wl;
        iwl = m_rep->iwl;
        sign = m_rep->sign;
    }

    const FixedPointValue FixedPointValue::quantization(const FixedPointParameters& params, bool& flag) const {
        // Simplified quantization - just return the value
        flag = false;
        return *this;
    }

    const FixedPointValue FixedPointValue::overflow(const FixedPointParameters& params, bool& flag) const {
        // Simplified overflow - just return the value
        flag = false;
        return *this;
    }

    const std::string FixedPointValue::to_string() const {
        return to_dec();
    }

    const std::string FixedPointValue::to_string(FixedPointEnums::NumRep) const {
        return to_dec();
    }

    const std::string FixedPointValue::to_string(FixedPointEnums::NumRep, bool) const {
        return to_dec();
    }

    const std::string FixedPointValue::to_string(FixedPointEnums::StringFormat) const {
        return to_dec();
    }

    const std::string FixedPointValue::to_string(FixedPointEnums::NumRep, FixedPointEnums::StringFormat) const {
        return to_dec();
    }

    const std::string FixedPointValue::to_string(FixedPointEnums::NumRep, bool, FixedPointEnums::StringFormat) const {
        return to_dec();
    }

} // namespace SystemVueModelBuilder
