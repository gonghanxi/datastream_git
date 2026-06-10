// FixedPointRep.h - Internal representation for FixedPoint
#pragma once

#include "FixedPointEnums.h"

namespace SystemVueModelBuilder
{
    // Internal representation using double for storage
    class FixedPointRep {
    public:
        double value;
        int wl;
        int iwl;
        FixedPointEnums::Sign sign;

        FixedPointRep()
            : value(0.0)
            , wl(FixedPointEnums::DEFAULT_WL_)
            , iwl(FixedPointEnums::DEFAULT_IWL_)
            , sign(FixedPointEnums::TWOS_COMPLEMENT)
        {}

        FixedPointRep(double v, int w, int iw, FixedPointEnums::Sign s)
            : value(v)
            , wl(w)
            , iwl(iw)
            , sign(s)
        {}

        FixedPointRep(const FixedPointRep& other)
            : value(other.value)
            , wl(other.wl)
            , iwl(other.iwl)
            , sign(other.sign)
        {}
    };
}
