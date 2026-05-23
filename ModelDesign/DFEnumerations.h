#ifndef DFENUMERATIONS_H
#define DFENUMERATIONS_H
#pragma once


namespace SystemVueModelBuilder {

    //查询 枚举
    enum QueryEnum
    {
        QUERY_NO = 0,
        QUERY_YES = 1
    };
    //转换 枚举
    enum SwitchEnum
    {
        SWITCH_OFF = 0,
        SWITCH_ON = 1
    };
    //判断 枚举
    enum BooleanEnum
    {
        BOOLEAN_FALSE = 0,
        BOOLEAN_TURE = 1
    };

    // extern MODELBUILDER_API const char * QUERY_ENUM;
    // extern MODELBUILDER_API const char * SWITCH_ENUM;
    // extern MODELBUILDER_API const char * BOOLEAN_ENUM;

    //重命名
    const char* const QUERY_ENUM = "QUERY_ENUM";
    const char* const SWITCH_ENUM = "SWITCH_ENUM";
    const char* const BOOLEAN_ENUM = "BOOLEAN_ENUM";

    namespace Units
    {
    //参数单位枚举
        enum UnitType
        {
            NONE,
            ANGLE,
            LENGTH,
            TIME,
            FREQUENCY,
            VOLTAGE,
            POWER,
            RESISTANCE,
            TEMPERATURE
        };
    }
        }
#endif // DFENUMERATIONS_H
