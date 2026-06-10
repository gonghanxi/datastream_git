#include "FixedPointBitRef.h"
#include "FixedPoint.h"

bool SystemVueModelBuilder::FixedPointBitRef::get() const
{
    return m_num.get_bit(m_idx);
}

void SystemVueModelBuilder::FixedPointBitRef::set(bool val)
{
    m_num.set_bit(m_idx, val);
}
