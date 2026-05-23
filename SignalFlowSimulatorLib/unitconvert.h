#ifndef UNITCONVERT_H
#define UNITCONVERT_H
#include <QString>
#include "algorithmmanager.h"
#include "VarExpressionParse.h"
#define M_PI 3.14159265358979323846
using namespace SystemVueModelBuilder;
class UnitConvert
{

private:


public:
    UnitConvert();
    ~UnitConvert();
    //前端值转换为标准单位值，传给模型
    static QString convertToStandardUnit(const QString& unitType, const QString& unit, const QString& dataType, const QString& value);
    static QString convertToStandardUnit(const QString& unitType, const QString& unit, const QString& value);
    //端口dataType string转成枚举类型
    static PortMsg::PortDataType convertToDataType(const QString& unit);

    //校验参数值的数据类型是否匹配声明类型
    static QString validateParameterType(const QString& declaredType, const QString& value);
    //校验包含变量的参数类型
    static QString validateParameterTypeWithVariables(const QString& declaredType,
                                                      const QString& value,
                                                      const QMap<QString, Variable>& variables);
    //校验枚举类型参数值
    static QString validateEnumeration(const QString& value, const QJsonArray& selectOptions);
    //检查Array标记与值的一致性
    static QString validateArrayConsistency(const QString& dataType, const QString& value);
    //根据unitType推断期望的数据类型
    static QString inferDataTypeFromUnitType(const QString& unitType);
    //解析数组元素
    static QStringList parseArrayElements(const QString& value);

    //解析complex参数
    static QPair<double, double> parseComplex(const QString &value);
    //判断字符串是否为有效的数组/矩阵格式
    static bool isValidArrayFormat(const QString& value);
    //fmi 数据类型转换为string
    static QString dataTypeToString(PortMsg::PortDataType dataType) {
        switch (dataType) {
            case PortMsg::PortDataType::REAL: return "real";
            case PortMsg::PortDataType::INT: return "integer";
            case PortMsg::PortDataType::COMPLEX: return "complex";
            case PortMsg::PortDataType::REAL_MATRIX: return "real_matrix";
            case PortMsg::PortDataType::INT_MATRIX: return "int_matrix";
            case PortMsg::PortDataType::COMPLEX_MATRIX: return "complex_matrix";
            default: return "real";
        }
    }
private:

    //判断字符串是否为有效整型
    static bool isValidInteger(const QString& value);
    //判断字符串是否为有效的浮点数
    static bool isValidDouble(const QString& value);
    //判断字符串是否为有效的布尔值
    static bool isValidBoolean(const QString& value);

    //判断字符串是否为有效的复数格式
    static bool isValidComplex(const QString& value);



    static QStringList splitArrayElements(const QString &row);//分割数组元素的通用函数（支持逗号和空格分隔）
    static bool isValidNumber(const QString &value);//检查是否是有效的数值

    //解析数组/矩阵的维度
    static QVector<int> parseArrayDimensions(const QString& value);

    //检查数组元素的数据类型一致性
    static QString validateArrayElementsType(const QStringList& elements, const QString& declaredElementType);

    //检查变量类型是否匹配参数类型
    static bool checkVariableTypeMatch(const Variable& var, const QString& paramType);

    //检查表达式中的变量类型
    static QString checkVariablesInExpression(const QString& expression,
                                              const QString& paramType,
                                              const QMap<QString, Variable>& variables);

    //从变量表中提取变量名
    static QStringList extractVariableNamesFromExpression(const QString& expression);

    //检查字符串是否包含运算符
    static bool containsOperators(const QString& expr);

};
#endif // UNITCONVERT_H
