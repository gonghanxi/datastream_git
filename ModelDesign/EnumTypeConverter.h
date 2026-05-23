#ifndef ENUMTYPECONVERTER_H
#define ENUMTYPECONVERTER_H

#include "DFInterface.h"
namespace SystemVueModelBuilder {
class EnumTypeConverter {
public:
    //系统枚举类型
    enum class SystemEnumType {
        QUERY_ENUM,
        BOOLEAN_ENUM,
        SWITCH_ENUM,
        CUSTOM_ENUM
    };

    //转换枚举类为int型的值
    static int ToInt(QueryEnum value)
    {
        return static_cast<int>(value);
    }
    static int ToInt(BooleanEnum value)
    {
        return static_cast<int>(value);
    }
    static int ToInt(SwitchEnum value)
    {
        return static_cast<int>(value);
    }

    //将int型转换为相应的枚举类型的值
    static QueryEnum ToQueryEnum(int value)
    {
        return static_cast<QueryEnum>(value);
    }
    static BooleanEnum ToBooleanEnum(int value)
    {
        return static_cast<BooleanEnum>(value);
    }
    static SwitchEnum ToSwitchEnum(int value)
    {
        return static_cast<SwitchEnum>(value);
    }

    //转换枚举值为string类型
    static std::string ToString(QueryEnum value)
    {
        switch(value) {
        case QUERY_NO:
            return "NO";
        case QUERY_YES:
            return "YSE";
        default:
            return "UNKOWN";
        }
    }
    static std::string ToString(BooleanEnum value)
    {
        switch(value) {
        case BOOLEAN_FALSE:
            return "FALSE";
        case BOOLEAN_TURE:
            return "TRUE";
        default:
            return "UNKOWN";
        }
    }
    static std::string ToString(SwitchEnum value)
    {
        switch(value) {
        case SWITCH_OFF:
            return "OFF";
        case SWITCH_ON:
            return "ON";
        default:
            return "UNKOWN";
        }
    }

    //获取到枚举类型的名称
    static std::string GetEnumTypeName(QueryEnum value)
    {
        return "QueryEnum";
    }
    static std::string GetEnumTypeName(BooleanEnum value)
    {
        return "BooleanEnum";
    }
    static std::string GetEnumTypeName(SwitchEnum value)
    {
        return "SwitchEnum";
    }
};
}
#endif // ENUMTYPECONVERTER_H
