#include "unitconvert.h"
#include <QMap>
#include <cmath>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

UnitConvert::UnitConvert()
{

}

UnitConvert::~UnitConvert()
{

}

QString UnitConvert::convertToStandardUnit(const QString& unitType, const QString& unit, const QString& dataType, const QString& value)
{
    // 判断dataType是否是matrix
    if(dataType.toLower().contains("array"))
    {
        //去掉首尾的方括号
        QString content = value;
        if(content.front() == '[' && content.back() == ']') {
            content = content.mid(1, content.length() - 2);
        }

//        qDebug() << "convertToStandardUnit content" << content;
        QString convertMatrix="[";
        QStringList contentList=content.split(";");
        for (int i=0;i<contentList.size();++i)
        {
            QString row=contentList.at(i);
            QStringList rowList=row.split(",");
            for (int j=0;j<rowList.size();++j)
            {
                QString convertValue=convertToStandardUnit(unitType,unit,rowList.at(j));
                convertMatrix.append(convertValue);
                if(j!=(rowList.size()-1))
                    convertMatrix.append(",");
            }
            if(i!=(contentList.size()-1))
                convertMatrix.append(";");
        }
        convertMatrix.append("]");
        return convertMatrix;
    }
    // 在数组处理的后面添加复数类型处理
    else if (dataType.toLower() == "complex") {
        // 解析复数
        QPair<double, double> complexVal = parseComplex(value);

        // 如果有单位转换，需要对实部和虚部分别进行单位转换
        QString realStr = QString::number(complexVal.first);
        QString imagStr = QString::number(complexVal.second);

        // 对实部进行单位转换
        QString convertedReal = convertToStandardUnit(unitType, unit, realStr);
        QString convertedImag = convertToStandardUnit(unitType, unit, imagStr);

        // 返回复数格式 (实部, 虚部)
        return QString("(%1, %2)").arg(convertedReal).arg(convertedImag);
    }
    else {
        return convertToStandardUnit(unitType,unit,value);
    }
}

// 单位转换函数：类型、原单位、数据类型、原始值 -> 标准单位值
QString UnitConvert::convertToStandardUnit(const QString& unitType, const QString& unit, const QString& value)
{
    qDebug() << "unitType" << unitType << ",unit" << unit << ",value" << value;
    // 1.单位类型：NONE
    if (unitType.compare("none", Qt::CaseInsensitive) == 0) {
        return value;
    }
    else {
        // 校验输入值是否为数字
        bool isNumber = false;
        double val = value.toDouble(&isNumber);
        if (!isNumber) {
            return QString("invalid_value");
        }

        // 2.频率单位转换（标准单位：Hz）
        if (unitType.compare("frequency", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> rateUnitFactor = {
                {"hz", 1.0},      // 赫兹（标准单位）
                {"khz", 1000.0},  // 千赫兹
                {"mhz", 1e6},     // 兆赫兹
                {"ghz", 1e9},     // 吉赫兹
                {"thz", 1e12}     // 太赫兹
            };

            if (!rateUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }
            double standardVal = val * rateUnitFactor[unit.toLower()];
            return QString::number(standardVal);
        }

        // 3.电阻单位转换（标准单位：Ohm）
        else if (unitType.compare("resistance", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> resistanceUnitFactor = {
                {"ohm", 1.0},     // 欧姆（标准单位）
                {"kohm", 1000.0}, // 千欧
                {"mohm", 1e6}     // 兆欧
            };
            if (!resistanceUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }
            double standardVal = val * resistanceUnitFactor[unit.toLower()];
            return QString::number(standardVal); // 整数/大数场景，不保留冗余小数
        }

        // 4.电感单位转换（标准单位：H）
        else if (unitType.compare("inductance", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> inductanceUnitFactor = {
                {"ph", 1e-12},    // 皮亨
                {"nh", 1e-9},     // 纳亨
                {"uh", 1e-6},     // 微亨
                {"mh", 1e-3},     // 毫亨
                {"h", 1.0}        // 亨利（标准单位）
            };

            if (!inductanceUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }

            double standardVal = val * inductanceUnitFactor[unit.toLower()];
//            return QString::number(standardVal, 'f', 15); // 适配小单位精度
            return QString::number(standardVal);
        }

        // 5.电容单位转换（标准单位：F）
        else if (unitType.compare("capacitance", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> capacitanceUnitFactor = {
                {"ff", 1e-15},     // 飞法
                {"pf", 1e-12},     // 皮法
                {"nf", 1e-9},      // 纳法
                {"uf", 1e-6},      // 微法
                {"mf", 0.001},     // 毫法
                {"f", 1.0}         // 法拉（标准单位）
            };
            if (!capacitanceUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }
            double standardVal = val * capacitanceUnitFactor[unit.toLower()];
//            return QString::number(standardVal, 'e', 18); // 保留18位小数，适配飞法极小单位
            return QString::number(standardVal);
        }

        // 6.长度单位转换（标准单位：m）
        else if (unitType.compare("length", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> lengthUnitFactor = {
                {"um", 1e-6},     // 微米
                {"mm", 0.001},    // 毫米
                {"cm", 0.01},     // 厘米
                {"m", 1.0}        // 米（标准单位）
            };
            if (!lengthUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }
            double standardVal = val * lengthUnitFactor[unit.toLower()];
//            return QString::number(standardVal, 'f', 9);
            return QString::number(standardVal);
        }

        // 7.时间单位转换（标准单位：s）
        else if (unitType.compare("time", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> timeUnitFactor = {
                {"ps", 1e-12},    // 皮秒
                {"ns", 1e-9},     // 纳秒
                {"us", 1e-6},     // 微秒
                {"ms", 0.001},    // 毫秒
                {"s", 1.0}        // 秒（标准单位）
            };

            if (!timeUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }
            double standardVal = val * timeUnitFactor[unit.toLower()];
//            return QString::number(standardVal, 'f', 15); // 保留15位小数，适配小单位
            return QString::number(standardVal);
        }

        // 8.角度单位转换（参考单位：rad）
        else if (unitType.compare("angle", Qt::CaseInsensitive) == 0) {
            static const double PI = M_PI;
            static const QMap<QString, double> angleUnitFactor = {
                {"deg", PI / 180.0},    // 度（1 deg = π/180 rad）
                {"rad", 1.0}            // 弧度（标准单位）
            };

            if (!angleUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }

            double standardVal = val * angleUnitFactor[unit.toLower()];
//            return QString::number(standardVal, 'f', 9);
            return QString::number(standardVal);
        }

        // 9. 电压单位转换（标准单位：V）
        else if (unitType.compare("voltage", Qt::CaseInsensitive) == 0) {
            // 线性电压单位→倍率映射（直接转换）
            static const QMap<QString, double> linearVoltFactor = {
                {"pv", 1e-12},    // 皮伏
                {"nv", 1e-9},     // 纳伏
                {"uv", 1e-6},     // 微伏
                {"mv", 1e-3},     // 毫伏
                {"v", 1.0},       // 伏（标准单位）
                {"kv", 1e3}       // 千伏
            };
            // 对数电压单位→基准值映射（用于对数转线性）
            static const QMap<QString, double> logVoltRef = {
                {"dbuv", 1e-6},   // dBuV 基准：1uV = 1e-6 V
                {"dbmv", 1e-3},   // dBmV 基准：1mV = 1e-3 V
                {"dbv", 1.0}      // dBV 基准：1V = 1.0 V
            };

            // 判断是线性还是对数电压单位
            if (linearVoltFactor.contains(unit.toLower())) {
                // 线性电压转换
                double standardVal = val * linearVoltFactor[unit.toLower()];
                return QString::number(standardVal, 'f', 15); // 保留15位小数适配小单位
            }
            else if (logVoltRef.contains(unit.toLower())) {
                // 对数电压转换（核心公式：V = 基准值 × 10^(dB值/20)）
                double ref = logVoltRef[unit.toLower()];
                double standardVal = ref * pow(10.0, val / 20.0);
//                return QString::number(standardVal, 'e', 9);
                return QString::number(standardVal);
            }
            else {
                // 不支持的电压单位
                return QString("invalid_value");
            }
        }

        // 10. 电流单位转换（标准单位：A）
        else if (unitType.compare("current", Qt::CaseInsensitive) == 0) {
            static const QMap<QString, double> currentUnitFactor = {
                {"pa", 1e-12},    // 皮安
                {"na", 1e-9},     // 纳安
                {"ua", 1e-6},     // 微安
                {"ma", 1e-3},     // 毫安
                {"a", 1.0}        // 安培（标准单位）
            };

            if (!currentUnitFactor.contains(unit.toLower())) {
                return QString("invalid_value");
            }

            // 计算并返回结果（保留15位小数适配皮安/纳安）
            double standardVal = val * currentUnitFactor[unit.toLower()];
//            return QString::number(standardVal, 'f', 15);
            return QString::number(standardVal);
        }

        // 11. 功率单位转换（标准单位：W）
        else if (unitType.compare("power", Qt::CaseInsensitive) == 0) {

            // 线性功率单位→倍率映射（转瓦特）
            static const QMap<QString, double> linearPowerFactor = {
                {"uW", 1e-6},     // 微瓦
                {"mW", 1e-3},     // 毫瓦
                {"W", 1.0},       // 瓦特（标准单位）
                {"KW", 1e3},      // 千瓦
                {"MW", 1e6}       // 兆瓦
            };
            // 对数功率单位→基准值映射（用于对数转线性）
            static const QMap<QString, double> logPowerRef = {
                {"dbm", 1e-3},    // dBm 基准：1mW = 1e-3 W
                {"dbw", 1.0}      // dBW 基准：1W = 1.0 W
            };

            // 第一步：判断是线性功率单位还是对数功率单位
            if (linearPowerFactor.contains(unit)) {
                double standardVal = val * linearPowerFactor[unit];
                return QString::number(standardVal); //由于有极小的数存在，所以不能只保留9位小数
//                return QString::number(standardVal, 'f', 9); // 保留9位小数适配微瓦/毫瓦
            }
            else if (logPowerRef.contains(unit)) {
                qDebug() << "log POWER enter this";
                double ref = logPowerRef[unit];
                // 核心公式：P(W) = 基准值 × 10^(dB值/10)
                double standardVal = ref * pow(10.0, val / 10.0);
                return QString::number(standardVal);
//                return QString::number(standardVal, 'e', 6); // 科学计数法适配宽范围值
            }
            // 不支持的功率单位
            else {
                return QString("invalid_value");
            }
        }

        // 12. 温度单位转换（标准：摄氏度）
        else if (unitType.compare("temperature", Qt::CaseInsensitive) == 0) {
            if (unit.compare("Celsius", Qt::CaseInsensitive) == 0) {
                return QString::number(val); // 摄氏度直接返回
            }
            else if (unit.compare("k", Qt::CaseInsensitive) == 0 || unit.compare("k°", Qt::CaseInsensitive) == 0) {
                // 开尔文转摄氏度：C = K - 273.15
                double cVal = val - 273.15;
                return QString::number(cVal, 'f', 6);
            }
            else {
                return QString("invalid_value");
            }
        }

        // 13. 相对值转换（标准单位：dB）
        else if (unitType.compare("relative", Qt::CaseInsensitive) == 0) {
            static const double REF_VOLTAGE = 1.0; // 电压参考值：1V（可自定义）

            double dbVal = 0.0;
            if (unit.toLower() == "db") {
                // dB直接返回原值
                dbVal = val;
            }
            else if (unit.toLower() == "abs") {
                // 电压绝对值 → dB（公式：20×lg(绝对值/参考值)）
                if (val <= 0) { // 电压不能为0/负数
                    return QString("invalid_value");
                }
                dbVal = 20.0 * log10(val / REF_VOLTAGE);
            }
            else {
                return QString("invalid_value");
            }

            // 保留3位小数，适配工程精度
            return QString::number(dbVal, 'f', 3);
        }

        // 14. 相对功率转换（标准：dB，仅处理功率维度）
        else if (unitType.compare("relative power", Qt::CaseInsensitive) == 0) {
            // 参考功率：默认1W（对应dBW基准，可改为1mW即1e-3对应dBm基准）
            static const double REF_POWER = 1.0;

            double dbVal = 0.0;
            // dB直接返回原值
            if (unit.toLower() == "db") {
                dbVal = val;
            }
            // 处理abs（功率绝对值）→ dB
            else if (unit.toLower() == "abs") {
                // 校验：功率绝对值不能≤0（对数函数无意义）
                if (val <= 0) {
                    return QString("invalid_value");
                }
                // 核心公式：相对功率dB = 10 × lg(功率绝对值 / 参考功率)
                dbVal = 10.0 * log10(val / REF_POWER);
            }
            else {
                return QString("invalid_value");
            }

            // 保留3位小数，适配工程精度
            return QString::number(dbVal, 'f', 3);
        }
    }

    // 不支持的单位类型
    return QString("invalid_value");
}

PortMsg::PortDataType UnitConvert::convertToDataType(const QString &dataType)
{
    QString dataTypeLower=dataType.toLower();

    static const QMap<QString,PortMsg::PortDataType> dataTypeFactor = {
        {"int", PortMsg::INT},
        {"complex", PortMsg::COMPLEX},
        {"anytype", PortMsg::ANYTYPE},
        {"envelope", PortMsg::ENVELOPE},
        {"real", PortMsg::REAL},
        {"fixedpoint", PortMsg::FIXEDPOINT},
        {"variant", PortMsg::VARIANT},
        {"multiple int", PortMsg::MULTIPLE_INT},
        {"multiple complex", PortMsg::MULTIPLE_COMPLEX},
        {"multiple anytype", PortMsg::MULTIPLE_ANYTYPE},
        {"multiple envelope", PortMsg::MULTIPLE_ENVELOPE},
        {"multiple real", PortMsg::MULTIPLE_REAL},
        {"multiple fixedpoint", PortMsg::MULTIPLE_FIXEDPOINT},
        {"multiple variant", PortMsg::MULTIPLE_VARIANT},
        {"int matrix", PortMsg::INT_MATRIX},
        {"complex matrix", PortMsg::COMPLEX_MATRIX},
        {"anytype matrix", PortMsg::ANYTYPE_MATRIX},
        {"envelope matrix", PortMsg::ENVELOPE_MATRIX},
        {"real matrix", PortMsg::REAL_MATRIX},
        {"fixedpoint matrix", PortMsg::FIXEDPOINT_MATRIX},
        {"variant matrix", PortMsg::VARIANT_MATRIX},
        {"multiple int matrix", PortMsg::MULTIPLE_INT_MATRIX},
        {"multiple complex matrix", PortMsg::MULTIPLE_COMPLEX_MATRIX},
        {"multiple anytype matrix", PortMsg::MULTIPLE_ANYTYPE_MATRIX},
        {"multiple envelope matrix", PortMsg::MULTIPLE_ENVELOPE_MATRIX},
        {"multiple real matrix", PortMsg::MULTIPLE_REAL_MATRIX},
        {"multiple fixedpoint matrix",PortMsg::MULTIPLE_FIXEDPOINT_MATRIX},
        {"multiple variant matrix",PortMsg::MULTIPLE_VARIANT_MATRIX}
    };
        return dataTypeFactor[dataTypeLower];

}

QString UnitConvert::validateParameterType(const QString &declaredType, const QString &value)
{
    QString lowerType = declaredType.toLower();

    //处理带array标记的类型
    if(lowerType.contains("array") || lowerType.contains("matrix")) {
        return "";//在validateArrayConsistency中检查
    }
    //根据声明的类型进行校验
    if(lowerType == "int" || lowerType == "integer") {
        if(!isValidInteger(value)) {
            return QString("参数声明的类型为整型，但值'%1'不是有效的整数").arg(value);
        }
    }
    else if (lowerType == "double" || lowerType == "float" ||
             lowerType == "real") {
        if (!isValidDouble(value)) {
            return QString("参数声明的类型为浮点型(double)，但值'%1'不是有效的浮点数").arg(value);
        }
    }
    else if (lowerType == "boolean" || lowerType == "bool") {
        if (!isValidBoolean(value)) {
            return QString("参数声明的类型为布尔型(boolean)，但值'%1'不是有效的布尔值").arg(value);
        }
    }
    else if (lowerType == "string" || lowerType == "text" ||
             lowerType == "char" || lowerType == "str") {
        // 字符串类型基本都有效，但可以检查一些特殊情况
        if (value.isEmpty() && lowerType != "string") {
            return QString("参数声明的类型为字符串，但值为空");
        }
    }
//    else if (lowerType == "complex") {
//        if (!isValidComplex(value)) {
//            return QString("参数声明的类型为复数(complex)，但值'%1'不是有效的复数格式").arg(value);
//        }
//    }
    else if (lowerType == "complex") {
        // 复数类型：支持实数、虚数、标准复数格式
        // 1. 检查是否是实数
        if (isValidDouble(value)) {
            return ""; // 实数可以作为复数（虚部为0）
        }

        // 2. 检查是否是纯虚数
        QRegularExpression pureImaginary("^([+-]?)(\\d*\\.?\\d+([eE][+-]?\\d+)?)?[ij]|^[ij]([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)$");
        if (pureImaginary.match(value.trimmed()).hasMatch()) {
            return "";
        }

        // 3. 检查是否是标准复数格式
        if (isValidComplex(value)) {
            return "";
        }

        // 4. 都不满足，才是错误
        return QString("参数声明的类型为复数(complex)，但值'%1'不是有效的复数格式（支持：实数、纯虚数、a+bi格式）").arg(value);
    }
    else if (lowerType == "variant" || lowerType == "anytype") {
        // Variant类型接受任何值
        return "";
    }
    else {
        // 未知类型，尝试推断
        if (!isValidDouble(value) && !isValidInteger(value)) {
            return QString("参数声明的类型为'%1'，值'%2'不是有效的数值格式").arg(declaredType).arg(value);
        }
    }

    return ""; // 空字符串表示校验通过

}

QString UnitConvert::validateParameterTypeWithVariables(const QString &declaredType, const QString &value, const QMap<QString, Variable> &variables)
{
    QString lowerType = declaredType.toLower();
     QString trimmedValue = value.trimmed();

     qDebug() << "校验带变量的参数: type=" << declaredType << ", value=" << trimmedValue;

     // 1. 检查是否包含变量引用
     if (!variables.isEmpty()) {
         QStringList varNames = extractVariableNamesFromExpression(trimmedValue);

         if (!varNames.isEmpty()) {
             qDebug() << "发现变量引用:" << varNames;

             // 检查所有变量是否都存在
             for (const QString& varName : varNames) {
                 if (!variables.contains(varName)) {
                     return QString("参数引用了未定义的变量: %1").arg(varName);
                 }
             }

             // 检查变量类型是否匹配参数类型
             QString varTypeCheck = checkVariablesInExpression(trimmedValue, declaredType, variables);
             if (!varTypeCheck.isEmpty()) {
                 return varTypeCheck;
             }

             // 如果表达式是纯变量（没有运算符），检查变量值本身
             if (varNames.size() == 1 && !containsOperators(trimmedValue)) {
                 Variable var = variables[varNames.first()];
                 QString varValue = var.defaultValue;

                 // 检查变量值的数据类型
                 return validateParameterType(declaredType, varValue);
             }

             // 如果是复杂表达式，返回空（表示通过检查，但实际计算由变量解析器完成）
             qDebug() << "复杂变量表达式，类型检查通过，具体值由变量解析器计算";
             return "";
         }
     }

     // 2. 如果没有变量引用或变量检查通过，进行常规类型检查
     return validateParameterType(declaredType, trimmedValue);
}

QString UnitConvert::validateEnumeration(const QString &value, const QJsonArray &selectOptions)
{
    // 检查值是否在选项列表中
    for (const QJsonValue& option : selectOptions) {
        if (option.isString() && option.toString() == value) {
            return ""; // 找到匹配项
        }
    }

    // 如果没有找到匹配项，构建错误信息
    QStringList optionList;
    for (const QJsonValue& option : selectOptions) {
        if (option.isString()) {
            optionList.append(option.toString());
        }
    }

    return QString("枚举值'%1'不在有效选项列表中。有效选项: %2")
           .arg(value).arg(optionList.join(", "));
}

QString UnitConvert::validateArrayConsistency(const QString &dataType, const QString &value)
{
    QString lowerType = dataType.toLower();

    // 检查是否声明为Array
    bool declaredAsArray = lowerType.contains("array") || lowerType.contains("matrix");
    bool valueIsArray = isValidArrayFormat(value);

    if (declaredAsArray && !valueIsArray) {
        // 提取数组元素类型
        QString elementType = "unknown";
        if (lowerType.contains("integer")) elementType = "整数";
        else if (lowerType.contains("double")) elementType = "浮点数";

        return QString("参数声明为%1数组类型('%2')，但值'%3'不是有效的数组格式")
               .arg(elementType).arg(dataType).arg(value);
    }

    if (!declaredAsArray && valueIsArray) {
        return QString("参数没有声明为数组类型('%1')，但值'%2'是数组格式")
               .arg(dataType).arg(value);
    }

    // 如果是数组，检查元素类型一致性
    if (declaredAsArray && valueIsArray) {
        QStringList elements = parseArrayElements(value);
        if (!elements.isEmpty()) {
            QString elementError = validateArrayElementsType(elements, dataType);
            if (!elementError.isEmpty()) {
                return elementError;
            }
        }
    }

    return ""; // 空字符串表示校验通过
}

QString UnitConvert::inferDataTypeFromUnitType(const QString &unitType)
{
    static const QMap<QString, QString> unitTypeToDataType = {
        //数值类型
        {"frequency", "double"},
        {"resistance", "double"},
        {"inductance", "double"},
        {"capacitance", "double"},
        {"length", "double"},
        {"time", "double"},
        {"angle", "double"},
        {"voltage", "double"},
        {"current", "double"},
        {"power", "double"},
        {"temperature", "double"},
        {"relative", "double"},
        {"relative power", "double"},

        //无单位类型
        {"none", "variant"},

        //
        {"string", "string"},
        {"boolean", "boolean"},
        {"integer", "int"},
        {"complex", "complex"}
    };

    QString lowerUnitType = unitType.toLower();
    if (unitTypeToDataType.contains(lowerUnitType)) {
        return unitTypeToDataType[lowerUnitType];
    }

    // 默认推断为double，因为大部分物理量都是数值
    return "double";
}

bool UnitConvert::isValidInteger(const QString &value)
{
    bool ok = false;
    value.toLongLong(&ok);
    if(ok) return true;

    //科学计数法表示的整数
    double d = value.toDouble(&ok);
    if(ok) {
        //检查是否为整型
        double intPart;
        return std::modf(d, &intPart) == 0.0;
    }
    return false;
}

bool UnitConvert::isValidDouble(const QString &value)
{
    bool ok = false;
    value.toDouble(&ok);
    return ok;
}

bool UnitConvert::isValidBoolean(const QString &value)
{
    QString lower = value.toLower();
    return (lower == "true" || lower == "false" ||
            lower == "1" || lower == "0" ||
            lower == "yes" || lower == "no");
}

bool UnitConvert::isValidComplex(const QString &value)
{
    QString trimmed = value.trimmed();

    // 1. 检查是否为纯实数
    if (isValidDouble(trimmed)) {
        return true;
    }

    // 2. 支持更灵活的复数格式
    // 新增支持：1+i, 1+j, 1+2*i, 1+2*j 等格式
    QRegularExpression complexRegex(
        // 格式1: a+bi 或 a-bi (i在数字后面)
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*(\\d*\\.?\\d+([eE][+-]?\\d+)?)[ij]$|"
        // 格式2: a+jb 或 a-ib (j/i在数字前面)
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*[ij](\\d*\\.?\\d+([eE][+-]?\\d+)?)$|"
        // 格式3: (a,b) 或 (a, b)
        "^\\s*\\(\\s*([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*,\\s*([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*\\)\\s*$|"
        // 格式4: a,b
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*,\\s*([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*$|"
        // 格式5: 纯虚数 j, i, 2j, j2, 2*i, i*2 等（新增）
        "^[+-]?\\s*[ij]|"
        "^[+-]?\\s*\\d*\\.?\\d+([eE][+-]?\\d+)?\\s*\\*?\\s*[ij]|"
        "^[ij]\\s*\\*?\\s*[+-]?\\s*\\d*\\.?\\d+([eE][+-]?\\d+)?|"
        // 格式6: a+ib 或 a+jb (i/j在数字前面，没有乘号) - 新增
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*[ij]|"
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*[ij]\\s*(\\d*\\.?\\d+([eE][+-]?\\d+)?)?$"
    );

    return complexRegex.match(trimmed).hasMatch();
}

bool UnitConvert::isValidArrayFormat(const QString &value)
{
    QString trimmed = value.trimmed();

    // 空数组
    if (trimmed == "[]") return true;

    // 必须用方括号包裹
    if (!(trimmed.startsWith('[') && trimmed.endsWith(']'))) {
//        qDebug() << "方括号失效";
        return false;
    }

    // 获取内部内容
    QString content = trimmed.mid(1, trimmed.length() - 2).trimmed();
    if (content.isEmpty()) return true;

    // 检查是否为矩阵（包含分号）
    if (content.contains(';')) {
        QStringList rows = content.split(';', QString::SkipEmptyParts);
        if (rows.isEmpty()) return false;

        // 检查第一行的元素格式
        QString firstRow = rows.first().trimmed();
        QStringList firstRowElements = splitArrayElements(firstRow);
        int firstColCount = firstRowElements.size();

        for (int i = 0; i < rows.size(); i++) {
            QString row = rows[i].trimmed();
            if (row.isEmpty()) return false;

            QStringList elements = splitArrayElements(row);

            // 检查列数一致性
            if (elements.size() != firstColCount) {
                qDebug() << "Row" << i << "has" << elements.size()
                         << "elements, expected" << firstColCount;
                return false;
            }

            // 检查每个元素是否有效
            for (const QString& elem : elements) {
                QString trimmedElem = elem.trimmed();
                if (trimmedElem.isEmpty()) return false;

                // 检查是否是有效的数值（包括科学计数法）
                if (!isValidNumber(trimmedElem)) {
                    return false;
                }
            }
        }
        return true;
    }
    // 一维数组
    else {
        QStringList elements = splitArrayElements(content);
//        qDebug() << "elements: " << elements;
        if (elements.isEmpty()) return false;

        // 检查每个元素是否有效
        for (const QString& elem : elements) {
            QString trimmedElem = elem.trimmed();
            if (trimmedElem.isEmpty()) return false;

//            qDebug() << "trimmedElem: " << trimmedElem;
            if (!isValidNumber(trimmedElem)) {
                qDebug() << "is not valid number";
                return false;
            }
        }
        return true;
    }
}

QStringList UnitConvert::splitArrayElements(const QString &row)
{
    QStringList elements;
    QString currentElement;
    bool inQuotes = false;
    int parenDepth = 0;    // 括号深度
    int bracketDepth = 0;  // 方括号深度

    for (int i = 0; i < row.length(); i++) {
        QChar ch = row[i];

        if (ch == '\"' || ch == '\'') {
            inQuotes = !inQuotes;
            currentElement.append(ch);
        }
        else if (ch == '(' && !inQuotes) {
            parenDepth++;
            currentElement.append(ch);
        }
        else if (ch == ')' && !inQuotes) {
            parenDepth--;
            currentElement.append(ch);
        }
        else if (ch == '[' && !inQuotes) {
            bracketDepth++;
            currentElement.append(ch);
        }
        else if (ch == ']' && !inQuotes) {
            bracketDepth--;
            currentElement.append(ch);
        }
        else if ((ch == ',' || ch == ' ' || ch == '\t') && !inQuotes && parenDepth == 0 && bracketDepth == 0) {
            // 分隔符：不在引号内，不在括号内，不在方括号内
            if (!currentElement.trimmed().isEmpty()) {
                elements.append(currentElement.trimmed());
            }
            currentElement.clear();
        }
        else {
            currentElement.append(ch);
        }
    }

    // 添加最后一个元素
    if (!currentElement.trimmed().isEmpty()) {
        elements.append(currentElement.trimmed());
    }

//    qDebug() << "splitArrayElements 结果:" << elements;
    return elements;
}

bool UnitConvert::isValidNumber(const QString &value)
{
    QString trimmed = value.trimmed();

    // 检查是否为有效的浮点数（包括科学计数法）
    bool ok = false;
    trimmed.toDouble(&ok);
    if (ok) return true;

    // 检查是否为有效的复数
    if (isValidComplex(trimmed)) return true;

    // 检查是否为有效的整数
    trimmed.toLongLong(&ok);
    if (ok) return true;

    // 检查是否为布尔值
    if (isValidBoolean(trimmed)) return true;

    return false;
}

QVector<int> UnitConvert::parseArrayDimensions(const QString &value)
{
    QVector<int> dimensions;

    if(!isValidArrayFormat(value)) {
        return dimensions;//空向量
    }

    QString trimmed = value.trimmed();
    QString content = trimmed.mid(1, trimmed.length() - 2).trimmed();

    if(content.isEmpty()) {
        dimensions.append(0);//空数组，维度为0
        return dimensions;
    }

    if(content.contains(';')) {
        //多维 行 × 列
        QStringList rows = content.split(';', QString::SkipEmptyParts);
        dimensions.append(rows.size());// 行数

        if(!rows.isEmpty()) {
            int ColCount = rows.first().count(',') + 1;
            dimensions.append(ColCount); // 列数
        }
    }
    else {
        //一维数组
        QStringList elements = content.split(',', QString::SkipEmptyParts);
        dimensions.append(elements.size());
    }
    return dimensions;
}

QStringList UnitConvert::parseArrayElements(const QString &value)
{
    QStringList elements;

     if (!isValidArrayFormat(value)) {
         return elements;
     }

     QString trimmed = value.trimmed();
     if (trimmed == "[]") return elements;

     QString content = trimmed.mid(1, trimmed.length() - 2).trimmed();

     if (content.contains(';')) {
         // 矩阵：展平所有元素
         QStringList rows = content.split(';', QString::SkipEmptyParts);
         for (const QString& row : rows) {
             elements.append(splitArrayElements(row.trimmed()));
         }
     } else {
         // 一维数组
         elements = splitArrayElements(content);
     }

     return elements;
}

QPair<double, double> UnitConvert::parseComplex(const QString &value)
{
    QString trimmed = value.trimmed();
    QPair<double, double> result(0.0, 0.0);

    qDebug() << "parseComplex 输入:" << trimmed;

    // 1. 如果是纯实数
    bool ok;
    double realVal = trimmed.toDouble(&ok);
    if (ok) {
        result.first = realVal;
        result.second = 0.0;
        qDebug() << "解析为纯实数:" << realVal;
        return result;
    }
    // 2. 处理特殊格式：1+i, 1+j, 1-i, 1-j
        QRegularExpression simpleComplex("^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*([ij])$");
        QRegularExpressionMatch simpleMatch = simpleComplex.match(trimmed);
        if (simpleMatch.hasMatch()) {
            double real = simpleMatch.captured(1).toDouble();
            QString sign = simpleMatch.captured(3);
            QString imagUnit = simpleMatch.captured(4);

            result.first = real;
            result.second = (sign == "+") ? 1.0 : -1.0;
            qDebug() << "2.parseComplex 返回:" << result.first << "+"  << "j" ;
            return result;
        }

        // 3. 处理格式：1+2*i 或 1+2*j
        QRegularExpression complexWithStar("^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*(\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*\\*\\s*([ij])$");
        QRegularExpressionMatch starMatch = complexWithStar.match(trimmed);
        if (starMatch.hasMatch()) {
            double real = starMatch.captured(1).toDouble();
            QString sign = starMatch.captured(3);
            double imagNum = starMatch.captured(4).toDouble();

            result.first = real;
            result.second = (sign == "+") ? imagNum : -imagNum;
            qDebug() << "3.parseComplex 返回:" << result.first << "+"  << result.second << "*j" ;
            return result;
        }

        // 4. 处理格式：1+i2 或 1+j2 (i/j在数字前面，没有乘号)
        QRegularExpression complexWithoutStar("^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*[ij]\\s*(\\d*\\.?\\d+([eE][+-]?\\d+)?)?$");
        QRegularExpressionMatch noStarMatch = complexWithoutStar.match(trimmed);
        if (noStarMatch.hasMatch()) {
            double real = noStarMatch.captured(1).toDouble();
            QString sign = noStarMatch.captured(3);
            QString imagNumStr = noStarMatch.captured(4);

            double imagNum = imagNumStr.isEmpty() ? 1.0 : imagNumStr.toDouble();
            result.first = real;
            result.second = (sign == "+") ? imagNum : -imagNum;
            qDebug() << "4.parseComplex 返回:" << result.first << "+"  << "j" << result.second;
            return result;
        }

    // 5. 如果是纯虚数（支持 j/i 在数字前面或后面）
    // 格式: j, 2j, j2, i, 3i, i3
    QRegularExpression pureImaginary("^([+-]?)(\\d*\\.?\\d+([eE][+-]?\\d+)?)?[ij]|^[ij]([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)$");
    QRegularExpressionMatch pureMatch = pureImaginary.match(trimmed);
    if (pureMatch.hasMatch()) {
        QString signStr = pureMatch.captured(1);
        QString numStr1 = pureMatch.captured(2);
        QString numStr2 = pureMatch.captured(4);
        QString numStr = !numStr1.isEmpty() ? numStr1 : numStr2;

        if (numStr.isEmpty()) {
            // 只有 j 或 i
            result.second = (signStr == "-") ? -1.0 : 1.0;
        } else {
            double num = numStr.toDouble();
            result.second = (signStr == "-") ? -num : num;
        }
        result.first = 0.0;
        qDebug() << "5.parseComplex 返回:" << result.second << "j/i";
        return result;
    }

    // 6. 标准复数格式: a+bi 或 a-bi 或 a+bj 或 a-bj
    // 也支持虚数符号在数字后面: a+jb 或 a-ib
    QRegularExpression standardComplex(
        // 格式: a + bj 或 a - bi (虚数符号在数字前面)
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*(\\d*\\.?\\d+([eE][+-]?\\d+)?)[ij]$|"
        // 格式: a + jb 或 a - ib (虚数符号在数字前面)
        "^([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*([+-])\\s*[ij](\\d*\\.?\\d+([eE][+-]?\\d+)?)$"
    );
    QRegularExpressionMatch standardMatch = standardComplex.match(trimmed);
    if (standardMatch.hasMatch()) {
        // 判断是哪一种格式
        if (!standardMatch.captured(1).isEmpty()) {
            // 格式1: a + bj 或 a - bi
            double real = standardMatch.captured(1).toDouble();
            QString sign = standardMatch.captured(3);
            double imag = standardMatch.captured(4).toDouble();

            result.first = real;
            result.second = (sign == "+") ? imag : -imag;
        } else {
            // 格式2: a + jb 或 a - ib
            double real = standardMatch.captured(6).toDouble();
            QString sign = standardMatch.captured(8);
            double imag = standardMatch.captured(9).toDouble();

            result.first = real;
            result.second = (sign == "+") ? imag : -imag;
        }
        qDebug() << "6.parseComplex 返回:" << result.first << "+" << result.second << "j";
        return result;
    }

    // 7. 格式: (a,b) 或 a,b
    QRegularExpression pairComplex("^\\s*\\(?\\s*([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*,\\s*([+-]?\\d*\\.?\\d+([eE][+-]?\\d+)?)\\s*\\)?\\s*$");
    QRegularExpressionMatch pairMatch = pairComplex.match(trimmed);
    if (pairMatch.hasMatch()) {
        result.first = pairMatch.captured(1).toDouble();
        result.second = pairMatch.captured(3).toDouble();
        qDebug() << "7.parseComplex 返回:" << "(" << result.first << "," << result.second << ")";
        return result;
    }

    // 如果解析失败，返回 (0,0)
    qDebug() << "0.parseComplex 返回:" << result.first << "+" << result.second << "j";
    return result;
}

QString UnitConvert::validateArrayElementsType(const QStringList &elements, const QString &declaredElementType)
{
    QString lowerType = declaredElementType.toLower();

    for (int i = 0; i < elements.size(); i++) {
        const QString& elem = elements[i];

        if (lowerType.contains("int") || lowerType.contains("integer")) {
            if (!isValidInteger(elem)) {
                return QString("数组元素[%1]='%2'不是有效的整数").arg(i).arg(elem);
            }
        }
        else if (lowerType.contains("double") || lowerType.contains("float") ||
                 lowerType.contains("real") || lowerType.contains("number")) {
            if (!isValidDouble(elem)) {
                return QString("数组元素[%1]='%2'不是有效的浮点数").arg(i).arg(elem);
            }
        }
        else if (lowerType == "boolean" || lowerType == "bool") {
            if (!isValidBoolean(elem)) {
                return QString("数组元素[%1]='%2'不是有效的布尔值").arg(i).arg(elem);
            }
        }
        else if (lowerType == "string" || lowerType == "text" ||
                 lowerType == "char") {
            // 字符串类型基本都有效，但可以检查一些特殊情况
            if (elem.isEmpty() && lowerType != "string") {
                return QString("数组元素声明的类型为字符串，但值为空");
            }
        }
        else if (lowerType == "complex") {
            if (!isValidComplex(elem)) {
                return QString("数组元素[%1]='%2'不是不是有效的复数格式").arg(i).arg(elem);
            }
        }
        else if (lowerType == "variant" || lowerType == "anytype") {
            // Variant类型接受任何值
            return "";
        }
        else {
            // 未知类型，尝试推断
            if (!isValidDouble(elem) && !isValidInteger(elem)) {
                return QString("数组元素声明的类型为'%1'，值'%2'不是有效的数值格式").arg(i).arg(elem);
            }
        }
    }

    return ""; // 空字符串表示成功
}

bool UnitConvert::checkVariableTypeMatch(const Variable &var, const QString &paramType)
{
    QString varType = var.dataType.toLower();
    QString paramLower = paramType.toLower();

    qDebug() << "检查变量类型匹配: varType=" << varType << ", paramType=" << paramLower;

    // 类型匹配规则
    if (paramLower == "variant" || paramLower == "anytype") {
        return true; // Variant类型接受任何变量类型
    }

    // 基本类型匹配
    if (paramLower.contains("int") || paramLower.contains("integer")) {
        return varType.contains("int") || varType.contains("integer") ||
               varType == "double" || varType == "float" || varType == "real";
    }

    if (paramLower.contains("double") || paramLower.contains("float") ||
        paramLower.contains("real") || paramLower.contains("number")) {
        return varType.contains("double") || varType.contains("float") ||
               varType.contains("real") || varType.contains("int") ||
               varType.contains("integer");
    }

    if (paramLower == "boolean" || paramLower == "bool") {
        return varType == "boolean" || varType == "bool";
    }

    if (paramLower == "string" || paramLower == "text" ||
        paramLower == "char" || paramLower == "str") {
        return varType.contains("string") || varType.contains("text") ||
               varType.contains("char");
    }

    if (paramLower == "complex") {
        return varType == "complex";
    }

    // 数组类型匹配
    if (paramLower.contains("array")) {
        return varType.contains("array");
    }

    // 默认情况下，允许数值类型之间的转换
    return (varType.contains("int") || varType.contains("double") ||
            varType.contains("float") || varType.contains("real")) &&
           (paramLower.contains("int") || paramLower.contains("double") ||
            paramLower.contains("float") || paramLower.contains("real"));
}

QString UnitConvert::checkVariablesInExpression(const QString &expression, const QString &paramType, const QMap<QString, Variable> &variables)
{
    QStringList varNames = extractVariableNamesFromExpression(expression);

    for (const QString& varName : varNames) {
        if (variables.contains(varName)) {
            Variable var = variables[varName];

            if (!checkVariableTypeMatch(var, paramType)) {
                return QString("变量 '%1' 的类型 '%2' 不匹配参数类型 '%3'")
                       .arg(varName).arg(var.dataType).arg(paramType);
            }
        }
    }

    return "";
}

QStringList UnitConvert::extractVariableNamesFromExpression(const QString &expression)
{
    QStringList varNames;

    // 匹配变量名：字母开头，可包含字母、数字、下划线
    QRegularExpression varRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b(?![(])");
    QRegularExpressionMatchIterator it = varRegex.globalMatch(expression);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString varName = match.captured(1);

        // 排除数学函数名
        static QStringList mathFunctions = {
            "sin", "cos", "tan", "asin", "acos", "atan",
            "sinh", "cosh", "tanh", "exp", "log", "log10",
            "sqrt", "abs", "floor", "ceil", "round"
        };

        // 排除常量名
        static QStringList constants = {"pi", "e"};

        // 检查是否是数字（排除纯数字）
        bool isNumber = false;
        varName.toDouble(&isNumber);

        if (!isNumber &&
            !mathFunctions.contains(varName) &&
            !constants.contains(varName) &&
            !varNames.contains(varName)) {
            varNames.append(varName);
        }
    }

    return varNames;
}

bool UnitConvert::containsOperators(const QString& expr)
{
    QString operators = "+-*/%^()";
    for (QChar op : operators) {
        if (expr.contains(op)) {
            return true;
        }
    }
    return false;
}
