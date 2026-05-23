#ifndef DATATYPESANDPARSERS_H
#define DATATYPESANDPARSERS_H

#include <string>
#include <complex>
#include <map>
#include <vector>
#include <variant>
#include <memory>
#include <iostream>
#include <QDebug>
#include "MatrixCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "DataTypes.h"

// 辅助模板用于获取 CircularBuffer 的元素类型
template<typename CircularBufferType>
struct circular_buffer_value_type;

//返回类型
template<typename T>
struct circular_buffer_value_type<SystemVueModelBuilder::CircularBuffer<T>> {
    using type = T;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::IntCircularBuffer> {
    using type = int;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::DoubleCircularBuffer> {
    using type = double;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::FloatCircularBuffer> {
    using type = float;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::BoolCircularBuffer> {
    using type = bool;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::FComplexCircularBuffer> {
    using type = std::complex<float>;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::DComplexCircularBuffer> {
    using type = std::complex<double>;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<int>> {
    using type = int;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<double>> {
    using type = double;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<float>> {
    using type = float;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<bool>> {
    using type = bool;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>> {
    using type = std::complex<float>;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>> {
    using type = std::complex<double>;
};

// 检查是否为总线类型的 trait
template<typename T>
struct is_circular_buffer_bus : std::false_type {};

template<typename T>
struct is_circular_buffer_bus<SystemVueModelBuilder::CircularBufferBusT<T>*> : std::true_type {};

template<typename T>
struct is_circular_buffer_bus<std::unique_ptr<SystemVueModelBuilder::CircularBufferBusT<T>>> : std::true_type {};


class DataTypesAndParsers
{
public:
    // 检查是否为总线类型
    static bool IsBusType(DataTypes::Type type);

    // 获取数据类型大小
    static size_t GetDataTypeSize(DataTypes::Type type);

    // 解析字符串为各种矩阵
    static SystemVueModelBuilder::Matrix<double> ParseStringToMatrixDouble(const std::string& str);
    static SystemVueModelBuilder::Matrix<int> ParseStringToMatrixInt(const std::string& str);
    static SystemVueModelBuilder::Matrix<bool> ParseStringToMatrixBool(const std::string& str);
    static SystemVueModelBuilder::Matrix<char> ParseStringToMatrixChar(const std::string& str);
    static SystemVueModelBuilder::Matrix<float> ParseStringToMatrixFloat(const std::string& str);
    static SystemVueModelBuilder::Matrix<std::complex<float>> ParseStringToMatrixFComplex(const std::string& str);
    static SystemVueModelBuilder::Matrix<std::complex<double>> ParseStringToMatrixDComplex(const std::string& str);

    // 数据类型转换辅助
    template<typename T>
    static DataTypes::Type GetDataTypeFromTemplate();

    // 类型兼容性检查
    template<typename T>
    static bool IsCompatibleType(DataTypes::Type dataType);

private:
    // 通用的字符串分割函数
    static std::vector<std::string> SplitString(const std::string& str, char delimiter);

    // 解析矩阵维度
    static std::pair<size_t, size_t> ParseMatrixDimensions(const std::string& str);
};

// 内联实现模板函数 - 获取数据类型
template<typename T>
inline DataTypes::Type DataTypesAndParsers::GetDataTypeFromTemplate() {
    if constexpr (std::is_same_v<T, int>) return DataType::INT;
    else if constexpr (std::is_same_v<T, double>) return DataType::DOUBLE;
    else if constexpr (std::is_same_v<T, float>) return DataType::FLOAT;
    else if constexpr (std::is_same_v<T, bool>) return DataType::BOOL;
    else if constexpr (std::is_same_v<T, std::complex<float>>) return DataType::COMPLEX_FLOAT;
    else if constexpr (std::is_same_v<T, std::complex<double>>) return DataType::COMPLEX_DOUBLE;

    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::IntCircularBuffer>) return DataType::CIRCULAR_BUFFER_INT;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::DoubleCircularBuffer>) return DataType::CIRCULAR_BUFFER_DOUBLE;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FloatCircularBuffer>) return DataType::CIRCULAR_BUFFER_FLOAT;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::BoolCircularBuffer>) return DataType::CIRCULAR_BUFFER_BOOL;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::DComplexCircularBuffer>) return DataType::CIRCULAR_BUFFER_DCOMPLEX;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FComplexCircularBuffer>) return DataType::CIRCULAR_BUFFER_FCOMPLEX;

    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<int>>) return DataType::TIMED_INT;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<double>>) return DataType::TIMED_DOUBLE;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<float>>) return DataType::TIMED_FLOAT;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<bool>>) return DataType::TIMED_BOOL;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>) return DataType::TIMED_FCOMPLEX;
    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>) return DataType::TIMED_DCOMPLEX;
    else return DataType::ANY;
}

template<typename T>
inline bool DataTypesAndParsers::IsCompatibleType(DataTypes::Type dataType) {
    if constexpr (std::is_same_v<T, double>) {
        return dataType == DataType::DOUBLE;
    } else if constexpr (std::is_same_v<T, float>) {
        return dataType == DataType::FLOAT;
    } else if constexpr (std::is_same_v<T, int>) {
        return dataType == DataType::INT;
    } else if constexpr (std::is_same_v<T, bool>) {
        return dataType == DataType::BOOL;
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        return dataType == DataType::COMPLEX_DOUBLE;
    } else if constexpr (std::is_same_v<T, std::complex<float>>) {
        return dataType == DataType::COMPLEX_FLOAT;
    } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeSignal>) {
        return dataType == DataType::ENVELOPE_SIGNAL;
    }

    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::IntCircularBuffer>) {
            return dataType == DataType::CIRCULAR_BUFFER_INT;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::DoubleCircularBuffer>) {
            return dataType == DataType::CIRCULAR_BUFFER_DOUBLE;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FloatCircularBuffer>) {
            return dataType == DataType::CIRCULAR_BUFFER_FLOAT;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::BoolCircularBuffer>) {
            return dataType == DataType::CIRCULAR_BUFFER_BOOL;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::DComplexCircularBuffer>) {
            return dataType == DataType::CIRCULAR_BUFFER_DCOMPLEX;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FComplexCircularBuffer>) {
            return dataType == DataType::CIRCULAR_BUFFER_FCOMPLEX;
        }

    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<int>>) {
            return dataType == DataType::TIMED_INT;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<double>>) {
            return dataType == DataType::TIMED_DOUBLE;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<float>>) {
            return dataType == DataType::TIMED_FLOAT;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<bool>>) {
            return dataType == DataType::TIMED_BOOL;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>) {
            return dataType == DataType::TIMED_FCOMPLEX;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>) {
            return dataType == DataType::TIMED_DCOMPLEX;
        }

    else if constexpr (std::is_same_v<T, SystemVueModelBuilder::IntMatrixCircularBuffer>) {
            return dataType == DataType::MATRIX_INT;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::DoubleMatrixCircularBuffer>) {
            return dataType == DataType::MATRIX_DOUBLE;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FloatMatrixCircularBuffer>) {
            return dataType == DataType::MATRIX_FLOAT;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::BoolMatrixCircularBuffer>) {
            return dataType == DataType::MATRIX_BOOL;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FComplexMatrixCircularBuffer>) {
            return dataType == DataType::MATRIX_FCOMPLEX;
        } else if constexpr (std::is_same_v<T, SystemVueModelBuilder::DComplexMatrixCircularBuffer>) {
            return dataType == DataType::MATRIX_DCOMPLEX;
        }
    return false;
}

#endif // DATATYPESANDPARSERS_H
