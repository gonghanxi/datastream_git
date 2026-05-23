#ifndef FIXEDPOINTENUMS_H
#define FIXEDPOINTENUMS_H
#pragma once
namespace SystemVueModelBuilder {
    namespace FixedPointEnums
    {
        //  用于字符串转换的数字表示形式枚举
        enum NumRep
        {
            NOBASE = 0,
            BINARY = 2,
            OCTAL = 8,
            DECIMAL = 10,
            HEX = 16,
            BINARY_UNSIGNED,
            BINARY_SIGN_MAGNITUDE,
            OCTAL_UNSIGNED,
            OCTAL_SIGN_MAGNITUDE,
            HEX_UNSIGNED,
            HEX_SIGN_MAGNITUDE,
            CSD
        };
        //  用于字符串转换的格式枚举
        enum StringFormat
        {
            FIXED_FORMAT,
            SCIENTIFIC_FORMAT
        };
        //  量化模式的枚举
        enum QuantizationMode
        {
            ROUND,
            ROUND_ZERO,
            ROUND_MINUS_INFINITY,
            ROUND_INFINITY,
            ROUND_CONVERGENT,
            TRUNCATE,
            TRUNCATE_ZERO
        };
        //  溢出模式的枚举
        enum OverflowMode
        {
            SATURATE,
            SATURATE_ZERO,
            SATURATE_SYMMETRICAL,
            WRAP,
            WRAP_SIGN_MAGNITUDE
        };
        //  符号编码的枚举
        enum Sign
        {
            UNSIGNED,
            TWOS_COMPLEMENT
        };
        //  内置和默认的定点类型参数值
        const int  DEFAULT_WL_ = 32;
        const int  DEFAULT_IWL_ = 32;
        const QuantizationMode DEFAULT_QUANTIZATION_MODE = TRUNCATE;
        const OverflowMode DEFAULT_OVERFLOW_MODE = FixedPointEnums::WRAP;
        const int  DEFAULT_N_BITS_ = 0;
    }
        }
#endif // FIXEDPOINTENUMS_H
