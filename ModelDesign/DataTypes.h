#ifndef DATATYPES_H
#define DATATYPES_H

#include <complex>
#include "EnvelopeSignal.h"

namespace DataTypes {
    enum class Type {
        INT,    //0
        DOUBLE, //1
        FLOAT,  //2
        BOOL,   //3
        COMPLEX_FLOAT, //4
        COMPLEX_DOUBLE,//5  //基础类型
        INT_ARRAY,     //6
        DOUBLE_ARRAY,  //7
        COMPLEX_DOUBLE_ARRAY,//8 //数组类型
        CIRCULAR_BUFFER_BASE,//9
        CIRCULAR_BUFFER_INT,//10
        CIRCULAR_BUFFER_DOUBLE,//11
        CIRCULAR_BUFFER_FLOAT, //12
        CIRCULAR_BUFFER_BOOL,  //13
        CIRCULAR_BUFFER_FCOMPLEX,//14
        CIRCULAR_BUFFER_DCOMPLEX,//15
        INT_BUS, //16
        DOUBLE_BUS,//17
        FLOAT_BUS, //18
        BOOL_BUS,  //19
        CHAR_BUS,  //20
        FCOMPLEX_BUS, //21
        DCOMPLEX_BUS, //22
        ENVELOPE_BUS, //23//总线类型
        ENVELOPE_SIGNAL, //24
        TIMED_INT, //25
        TIMED_DOUBLE, //26
        TIMED_FLOAT,  //27
        TIMED_BOOL,   //28
        TIMED_FCOMPLEX, //29
        TIMED_DCOMPLEX, //30//时域类型
        MATRIX_INT,//31
        MATRIX_DOUBLE,//32
        MATRIX_FLOAT,//33
        MATRIX_BOOL,//34
        MATRIX_FCOMPLEX,//35
        MATRIX_DCOMPLEX,//36
        MATRIX_ENVELOPE,//37//矩阵类型
        MATRIX_TIME_INT,//38
        MATRIX_TIME_DOUBLE,//39
        MATRIX_TIME_FLOAT,//40
        MATRIX_TIME_BOOL,//41
        MATRIX_TIME_FCOMPLEX,//42
        MATRIX_TIME_DCOMPLEX,//43//时域矩阵类型

        MATRIX_INT_BUS,//44
        MATRIX_DOUBLE_BUS,//45
        MATRIX_FLOAT_BUS,//46
        MATRIX_BOOL_BUS,//47
        MATRIX_FCOMPLEX_BUS,//48
        MATRIX_DCOMPLEX_BUS,//49
        MATRIX_ENVELOPE_BUS,//50

        // 定点数类型
        FIXED_POINT,              //52 基础定点数类型
        FIXED_POINT_ARRAY,        //53 定点数数组
        CIRCULAR_BUFFER_FIXED_POINT, //54 定点数循环缓冲区
        FIXED_POINT_BUS,          //55 定点数总线
        TIMED_FIXED_POINT,        //56 时域定点数
        MATRIX_FIXED_POINT,       //57 定点数矩阵
        MATRIX_TIME_FIXED_POINT,  //58 时域定点数矩阵
        MATRIX_FIXED_POINT_BUS,   //59 定点数矩阵总线

        ANY //60
    };

    // 辅助函数
    bool IsBusType(Type type);
    size_t GetDataTypeSize(Type type);
}

// 为了方便，可以在全局作用域提供一个类型别名
using DataType = DataTypes::Type;

#endif // DATATYPES_H
