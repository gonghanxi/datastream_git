#include "VarExpressionParse.h"
#include "unitconvert.h"
#include <QStack>
#include <algorithm>
#include <QDebug>

// 定义数学常量（Windows平台需要）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

VarExpressionParse::VarExpressionParse()
{
    initializeMathFunctions();
}

VarExpressionParse::~VarExpressionParse()
{
    qDebug() << "~VarExpressionParse start";

//    qDebug() << "1. m_mathFunctions size:" << m_mathFunctions.size();
    m_mathFunctions.clear();
    qDebug() << "m_mathFunctions cleared";

//    qDebug() << "2. m_constants size:" << m_constants.size();
    m_constants.clear();
//    qDebug() << "m_constants cleared";

//    qDebug() << "3. m_contextVariables size:" << m_contextVariables.size();
    for (auto it = m_contextVariables.begin(); it != m_contextVariables.end(); ++it) {
//        qDebug() << "  Context:" << it.key() << "size:" << it.value().size();
    }
    m_contextVariables.clear();
//    qDebug() << "m_contextVariables cleared";

//    qDebug() << "4. m_variables size:" << m_variables.size();
    m_variables.clear();
//    qDebug() << "m_variables cleared";

    qDebug() << "~VarExpressionParse end";
}

void VarExpressionParse::setVariables(const QVector<Variable> &variables)
{
    //通过变量结构体容器将变量存入变量表
    //key: 变量名 value: 变量
    m_variables.clear();
    for (const Variable& var : variables) {
        m_variables[var.name] = var;
    }
}

void VarExpressionParse::setVariables(const QJsonArray &varsArray)
{
    m_variables.clear();

    //通过解析链路json文件传过来的vars字段，设置变量表
    for (const QJsonValue& varValue : varsArray) {
        if (varValue.isObject()) {
            QJsonObject varObj = varValue.toObject();

            Variable var;
            var.name = varObj["name"].toString();
            var.dataType = varObj["dataType"].toString();
            var.defaultValue = varObj["defaultValue"].toString();
            var.unit = varObj["unit"].toString();
            var.unitType = varObj["unitType"].toString();
            var.constraint = varObj["constraint"].toString();
            var.desc = varObj["desc"].toString();
            var.id = varObj["id"].toString();
            var.disp = varObj["disp"].toBool(true);

            // 保存到变量表
            m_variables[var.name] = var;

            // 如果有context信息，按上下文保存
            QString contextId = varObj["contextId"].toString();
            if (!contextId.isEmpty()) {
                m_contextVariables[contextId][var.name] = var;
            }
        }
    }
}

ExpressionResult VarExpressionParse::parseExpression(const QString &expression)
{
    ExpressionResult result;
    result.expression = expression;

    QString expr = expression.trimmed();

    // 空表达式
    if (expr.isEmpty()) {
        result.value = "";
        result.success = true;
        return result;
    }

    // 第一步：检查是否是数组表达式
    if (isArrayExpression(expr)) {
        ExpressionResult arrayResult = parseArrayWithExpressions(expr);
        if (arrayResult.success) {
            return arrayResult;
        }
    }

    // 第二步：优先尝试解析为复数（在检查变量引用之前）
    // 因为像 "a + bj" 这样的表达式可能被误认为是变量
    qDebug() << "尝试解析表达式:" << expr;

    // 检查是否是复数表达式
    bool couldBeComplex = expr.contains('j') || expr.contains('i') ||
                         (expr.contains('(') && expr.contains(',') && expr.contains(')'));

    if (couldBeComplex) {
        qDebug() << "可能是复数表达式，尝试解析...";
        QPair<double, double> complexVal = UnitConvert::parseComplex(expr);

        // 检查是否成功解析为非零值
        if (complexVal.first != 0.0 || complexVal.second != 0.0) {
            // 使用 'g' 格式保留最多精度，避免四舍五入
            result.value = QString("(%1, %2)")
                          .arg(complexVal.first, 0, 'g', 15)
                          .arg(complexVal.second, 0, 'g', 15);
            result.success = true;
            qDebug() << "复数解析成功:" << expr << "->" << result.value;
            return result;
        } else {
            qDebug() << "复数解析失败，继续其他解析方式";
        }
    }

    // 第三步：检查是否包含变量引用
    QStringList varNames = extractVariableNames(expr);
    if (!varNames.isEmpty()) {
        // 构建变量值映射
        QMap<QString, QString> varValues;
        for (const QString& varName : varNames) {
            if (m_variables.contains(varName)) {
                Variable var = m_variables[varName];
                QVariant varVal = parseVariableValue(var);
                varValues[varName] = variableValueToString(varVal, var.dataType);
            } else {
                // 检查是否是内置常量（如 pi）
                QString lowerVarName = varName.toLower();
                if (m_constants.contains(lowerVarName)) {
                    // 是内置常量
                    varValues[varName] = QString::number(m_constants[lowerVarName]);
                } else {
                    result.error = QString("未定义的变量或常量: %1").arg(varName);
                    result.success = false;
                    return result;
                }
            }
        }

        // 替换变量
        QString replacedExpr = replaceVariables(expr, varValues);

        // 递归解析替换后的表达式（注意避免无限递归）
        if (replacedExpr != expr) {
            return parseExpression(replacedExpr);
        }
    }

    // 第四步：尝试计算数学表达式
    ExpressionResult evalResult = evaluateMathExpression(expr);
    if (evalResult.success) {
        return evalResult;
    }

    // 第五步：再次尝试复数解析（以防前面步骤改变了表达式）
    if (couldBeComplex) {
        QPair<double, double> complexVal = UnitConvert::parseComplex(expr);
        if (complexVal.first != 0.0 || complexVal.second != 0.0) {
            result.value = QString("(%1, %2)").arg(complexVal.first).arg(complexVal.second);
            result.success = true;
            qDebug() << "最终复数解析成功:" << expr << "->" << result.value;
            return result;
        }
    }

    // 第六步：如果不是数学表达式，返回原始字符串
    result.value = expr;
    result.success = true;
    qDebug() << "返回原始表达式:" << expr;
    return result;
}

QString VarExpressionParse::parseParameterValue(const QString &paramValue)
{
    ExpressionResult result = parseExpression(paramValue);
    if (result.success) {
        return result.value;
    } else {
        qDebug() << "解析参数值失败:" << paramValue << "错误:" << result.error;
        return paramValue;  // 返回原始值
    }
}

QMap<QString, Variable> VarExpressionParse::getAllVariables() const
{
    return m_variables;
}

QMap<QString, Variable> VarExpressionParse::getContextVariables(const QString &contextId) const
{
    if (contextId.isEmpty() || !m_contextVariables.contains(contextId)) {
        return m_variables;
    }
    return m_contextVariables[contextId];
}

void VarExpressionParse::clear()
{
    m_variables.clear();
    m_contextVariables.clear();
}

ExpressionResult VarExpressionParse::parseArrayExpression(const QString &expression)
{
    // 先尝试使用新的解析器
    ExpressionResult result = parseArrayWithExpressions(expression);
    if (result.success) {
        return result;
    }

    // 如果失败，回退到原来的逻辑
    return parseArrayScalarOperation(expression);
}

ExpressionResult VarExpressionParse::parseArrayWithExpressions(const QString &expression)
{
    ExpressionResult result;
      result.expression = expression;

      QString expr = expression.trimmed();
      QString content = expr.mid(1, expr.length() - 2).trimmed();

      if (content.isEmpty()) {
          result.value = "[]";
          result.success = true;
          return result;
      }

      // 检查是否是矩阵（包含分号）
      if (content.contains(';')) {
          return parseMatrixExpression(content);
      }

      // 一维数组
      return parseOneDimensionalArray(content);
}

ExpressionResult VarExpressionParse::parseOneDimensionalArray(const QString &content)
{
    ExpressionResult result;
    result.expression = "[" + content + "]";

    QStringList elements = splitArrayContent(content);
    QStringList parsedElements;

    for (int i = 0; i < elements.size(); i++) {
        QString element = elements[i].trimmed();

        if (element == ";") {
            // 这是矩阵分隔符，不应该出现在一维数组中
            result.success = false;
            result.error = "一维数组中不应包含矩阵分隔符 ';'";
            return result;
        }

        // 调试：打印原始元素
        qDebug() << "解析数组元素[" << i << "]:" << element;

        // 递归解析每个元素
        ExpressionResult elementResult = parseExpression(element);
        qDebug() << "元素解析结果:" << elementResult.success << "值:" << elementResult.value;

        if (!elementResult.success) {
            // 如果解析失败，尝试直接作为复数解析
            qDebug() << "尝试直接解析为复数:" << element;
            QPair<double, double> complexVal = UnitConvert::parseComplex(element);

            if (complexVal.first != 0.0 || complexVal.second != 0.0) {
                // 成功解析为复数
                elementResult.value = QString("(%1, %2)").arg(complexVal.first).arg(complexVal.second);
                elementResult.success = true;
                qDebug() << "复数解析成功:" << element << "->" << elementResult.value;
            } else {
                result.success = false;
                result.error = QString("数组元素[%1]解析失败: %2").arg(i).arg(element);
                return result;
            }
        }

        parsedElements.append(elementResult.value);
    }

    // 重新构建数组
    result.value = "[" + parsedElements.join(", ") + "]";
    result.success = true;
    qDebug() << "最终数组结果:" << result.value;
    return result;
}

ExpressionResult VarExpressionParse::parseMatrixExpression(const QString &content)
{
    ExpressionResult result;
    result.expression = "[" + content + "]";

    QStringList rows = content.split(';', QString::SkipEmptyParts);
    QStringList parsedRows;

    for (int i = 0; i < rows.size(); i++) {
        QString row = rows[i].trimmed();

        // 解析每一行
        ExpressionResult rowResult = parseOneDimensionalArray(row);
        if (!rowResult.success) {
            result.success = false;
            result.error = QString("矩阵第%1行解析失败: %2").arg(i).arg(rowResult.error);
            return result;
        }

        // 去掉外层括号
        QString rowContent = rowResult.value.mid(1, rowResult.value.length() - 2);
        parsedRows.append(rowContent);
    }

    // 重新构建矩阵
    result.value = "[" + parsedRows.join("; ") + "]";
    result.success = true;
    return result;
}

QStringList VarExpressionParse::splitArrayContent(const QString &content)
{
    QStringList elements;
    QString currentElement;
    int bracketDepth = 0;  // 括号深度
    bool inQuotes = false; // 是否在引号内

    for (int i = 0; i < content.length(); i++) {
        QChar ch = content[i];

        if (ch == '\"' || ch == '\'') {
            inQuotes = !inQuotes;
            currentElement.append(ch);
        }
        else if (ch == '(') {
            bracketDepth++;
            currentElement.append(ch);
        }
        else if (ch == ')') {
            bracketDepth--;
            currentElement.append(ch);
        }
        else if (ch == '[') {
            bracketDepth++;  // 支持嵌套数组
            currentElement.append(ch);
        }
        else if (ch == ']') {
            bracketDepth--;
            currentElement.append(ch);
        }
        else if (ch == ',' && bracketDepth == 0 && !inQuotes) {
            // 分隔符
            if (!currentElement.trimmed().isEmpty()) {
                elements.append(currentElement.trimmed());
            }
            currentElement.clear();
        }
        else if (ch == ';' && bracketDepth == 0 && !inQuotes) {
            // 矩阵行分隔符
            if (!currentElement.trimmed().isEmpty()) {
                elements.append(currentElement.trimmed());
                elements.append(";");  // 特殊标记表示行结束
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

    return elements;
}

bool VarExpressionParse::isArrayExpression(const QString &expression)
{
    QString expr = expression.trimmed();

    if (expr.isEmpty() || !expr.startsWith('[') || !expr.endsWith(']')) {
        return false;
    }

    // 检查内部内容
    QString content = expr.mid(1, expr.length() - 2).trimmed();
    if (content.isEmpty()) {
        return true;  // 空数组
    }

    // 尝试分割数组元素
    QStringList elements = splitArrayContent(content);

    // 只要至少有一个元素，就认为是数组
    return !elements.isEmpty();
}

void VarExpressionParse::initializeMathFunctions()
{
    // 数学函数
    m_mathFunctions["sin"] = [](double x) { return std::sin(x); };
    m_mathFunctions["cos"] = [](double x) { return std::cos(x); };
    m_mathFunctions["tan"] = [](double x) { return std::tan(x); };
    m_mathFunctions["asin"] = [](double x) { return std::asin(x); };
    m_mathFunctions["acos"] = [](double x) { return std::acos(x); };
    m_mathFunctions["atan"] = [](double x) { return std::atan(x); };
    m_mathFunctions["sinh"] = [](double x) { return std::sinh(x); };
    m_mathFunctions["cosh"] = [](double x) { return std::cosh(x); };
    m_mathFunctions["tanh"] = [](double x) { return std::tanh(x); };
    m_mathFunctions["exp"] = [](double x) { return std::exp(x); };
    m_mathFunctions["log"] = [](double x) { return std::log(x); };
    m_mathFunctions["log10"] = [](double x) { return std::log10(x); };
    m_mathFunctions["sqrt"] = [](double x) { return std::sqrt(x); };
    m_mathFunctions["abs"] = [](double x) { return std::abs(x); };
    m_mathFunctions["floor"] = [](double x) { return std::floor(x); };
    m_mathFunctions["ceil"] = [](double x) { return std::ceil(x); };
    m_mathFunctions["round"] = [](double x) { return std::round(x); };

    // 常量
    m_constants["pi"] = M_PI;
    m_constants["e"] = M_E;

    m_constants["inf"] = std::numeric_limits<double>::infinity();
    m_constants["nan"] = std::numeric_limits<double>::quiet_NaN();
}

QStringList VarExpressionParse::extractVariableNames(const QString &expression)
{
    QStringList varNames;

     // 匹配变量名
     QRegularExpression varRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b(?![(])");
     QRegularExpressionMatchIterator it = varRegex.globalMatch(expression);

     // 大小写不敏感的常量列表
     static QStringList caseInsensitiveConstants = {"pi", "e", "inf", "nan"};

     while (it.hasNext()) {
         QRegularExpressionMatch match = it.next();
         QString varName = match.captured(1);
         QString lowerVarName = varName.toLower();

         // 排除数学函数名
         if (m_mathFunctions.contains(lowerVarName)) {
             continue;
         }

         // 排除常量（大小写不敏感）
         if (caseInsensitiveConstants.contains(lowerVarName) ||
             m_constants.contains(lowerVarName)) {
             continue;
         }

         // 排除复数单位（大小写不敏感）
         static QStringList complexUnits = {"i", "j"};
         if (complexUnits.contains(lowerVarName)) {
             continue;
         }

         // 检查是否是数字
         bool isNumber = false;
         varName.toDouble(&isNumber);
         if (!isNumber && !varNames.contains(varName)) {
             varNames.append(varName);
         }
     }

     return varNames;
}

QString VarExpressionParse::replaceVariables(const QString &expression, const QMap<QString, QString> &varValues)
{
    QString result = expression;

    // 按变量名长度降序排序，避免替换部分匹配的问题
    QList<QString> sortedKeys = varValues.keys();
    std::sort(sortedKeys.begin(), sortedKeys.end(),
              [](const QString& a, const QString& b) { return a.length() > b.length(); });

    for (const QString& varName : sortedKeys) {
        // 使用单词边界确保只匹配完整的变量名
        QRegularExpression pattern(QString("\\b%1\\b").arg(QRegularExpression::escape(varName)));
        result.replace(pattern, varValues[varName]);
    }

    return result;
}

ExpressionResult VarExpressionParse::evaluateMathExpression(const QString &expression)
{
    ExpressionResult result;
    result.expression = expression;

    bool ok = false;
    double value = parseArithmeticExpression(expression, ok);

    if (ok) {
        result.value = QString::number(value, 'g', 15);
        result.success = true;
    } else {
        result.error = "数学表达式计算失败";
        result.success = false;
    }

    return result;
}

QVariant VarExpressionParse::parseVariableValue(const Variable &var)
{
    QString dataType = var.dataType.toLower();
    QString value = var.defaultValue;

    if (dataType.contains("int") || dataType.contains("integer")) {
        int intValue;
        if (safeToInt(value, intValue)) {
            return QVariant(intValue);
        }
    }
    else if (dataType.contains("double") || dataType.contains("float") ||
             dataType.contains("real") || dataType.contains("number")) {
        double doubleValue;
        if (safeToDouble(value, doubleValue)) {
            return QVariant(doubleValue);
        }
    }
    else if (dataType == "boolean" || dataType == "bool") {
        QString lower = value.toLower();
        if (lower == "true" || lower == "1" || lower == "yes") {
            return QVariant(true);
        } else {
            return QVariant(false);
        }
    }
    else if (dataType == "string" || dataType == "text") {
        return QVariant(value);
    }

    // 默认返回字符串
    return QVariant(value);
}

QString VarExpressionParse::variableValueToString(const QVariant &value, const QString &dataType)
{
    QString lowerDataType = dataType.toLower();
    if (lowerDataType.contains("int") || lowerDataType.contains("integer")) {
        return QString::number(value.toInt());
    }
    else if (lowerDataType.contains("double") || lowerDataType.contains("float") ||
             lowerDataType.contains("real")) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    else if (lowerDataType == "boolean" || lowerDataType == "bool") {
        return value.toBool() ? "true" : "false";
    }
    else {
        return value.toString();
    }
}

bool VarExpressionParse::safeToDouble(const QString& str, double& result)
{
    bool ok = false;
    result = str.toDouble(&ok);
    return ok;
}

bool VarExpressionParse::safeToInt(const QString& str, int& result)
{
    bool ok = false;
    result = str.toInt(&ok);
    return ok;
}

int VarExpressionParse::getOperatorPriority(const QChar &op)
{
    switch (op.unicode()) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
        case '%':
            return 2;
        case '^':
            return 3;
        default:
            return 0;
    }
}

double VarExpressionParse::parseArithmeticExpression(const QString &expr, bool &ok)
{
    ok = true;
    QString expression = expr.trimmed();

    // 移除外层括号
    while (expression.startsWith('(') && expression.endsWith(')')) {
        expression = expression.mid(1, expression.length() - 2).trimmed();
    }

    // 检查是否是数值
    double numValue;
    if (safeToDouble(expression, numValue)) {
        return numValue;
    }

    // 处理数学函数
    QRegularExpression funcRegex("^([a-zA-Z_][a-zA-Z0-9_]*)\\(([^)]+)\\)$");
    QRegularExpressionMatch match = funcRegex.match(expression);
    if (match.hasMatch()) {
        QString funcName = match.captured(1);
        QString argsStr = match.captured(2);

        if (m_mathFunctions.contains(funcName)) {
            // 单参数函数
            double arg = parseArithmeticExpression(argsStr, ok);
            if (!ok) return 0.0;
            return m_mathFunctions[funcName](arg);
        }
    }

    // 处理常量 - 改为大小写不敏感
    QString lowerExpr = expression.toLower();
    if (m_constants.contains(lowerExpr)) {
        return m_constants[lowerExpr];
    }

    // 运算符优先级处理
    int parenCount = 0;
    int lowestPriority = INT_MAX;
    int lowestPos = -1;
    QChar lowestOp;

    for (int i = expression.length() - 1; i >= 0; --i) {
        QChar ch = expression[i];

        if (ch == ')') parenCount++;
        else if (ch == '(') parenCount--;
        else if (parenCount == 0) {
            int priority = getOperatorPriority(ch);
            if (priority > 0 && priority <= lowestPriority) {
                lowestPriority = priority;
                lowestPos = i;
                lowestOp = ch;
            }
        }
    }

    if (lowestPos != -1) {
        QString left = expression.left(lowestPos).trimmed();
        QString right = expression.mid(lowestPos + 1).trimmed();

        double leftVal = parseArithmeticExpression(left, ok);
        if (!ok) return 0.0;

        double rightVal = parseArithmeticExpression(right, ok);
        if (!ok) return 0.0;

        return evaluateBinaryOp(leftVal, rightVal, lowestOp);
    }

    ok = false;
    return 0.0;
}

double VarExpressionParse::evaluateBinaryOp(double left, double right, QChar op)
{
    switch (op.unicode()) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/':
            if (right == 0) return 0;
            return left / right;
        case '%':
            if (right == 0) return 0;
            return std::fmod(left, right);
        case '^': return std::pow(left, right);
        default: return 0;
    }
}

QString VarExpressionParse::performArrayOperation(const QString &arrayStr, const QString &operation)
{
    // 解析数组元素
    QStringList elements = UnitConvert::parseArrayElements(arrayStr);

    // 解析操作
    QChar op = operation[0];
    QString scalarStr = operation.mid(1);
    double scalar = scalarStr.toDouble();

    QStringList resultElements;

    for (const QString& elem : elements) {
        double value = elem.toDouble();
        double result = 0.0;

        switch (op.unicode()) {
            case '*': result = value * scalar; break;
            case '/': result = value / scalar; break;
            case '+': result = value + scalar; break;
            case '-': result = value - scalar; break;
            case '^': result = std::pow(value, scalar); break;
            default: result = value;
        }

        // 保留适当的精度
        resultElements.append(QString::number(result, 'g', 12));
    }

    // 重新构建数组
    return QString("[%1]").arg(resultElements.join(", "));
}

ExpressionResult VarExpressionParse::parseArrayScalarOperation(const QString &expression)
{
    ExpressionResult result;
     result.expression = expression;

     // 提取数组部分
     int arrayEnd = expression.indexOf(']');
     QString arrayStr = expression.left(arrayEnd + 1);

     // 验证数组格式
     if (!UnitConvert::isValidArrayFormat(arrayStr)) {
         result.success = false;
         result.error = "无效的数组格式";
         return result;
     }

     // 提取运算符和标量
     QString operationPart = expression.mid(arrayEnd + 1).trimmed();

     // 解析运算符和标量值
     QChar op;
     QString scalarStr;

     if (operationPart.startsWith('*')) {
         op = '*';
         scalarStr = operationPart.mid(1).trimmed();
     } else if (operationPart.startsWith('/')) {
         op = '/';
         scalarStr = operationPart.mid(1).trimmed();
     } else if (operationPart.startsWith('+')) {
         op = '+';
         scalarStr = operationPart.mid(1).trimmed();
     } else if (operationPart.startsWith('-')) {
         op = '-';
         scalarStr = operationPart.mid(1).trimmed();
     } else if (operationPart.startsWith('^')) {
         op = '^';
         scalarStr = operationPart.mid(1).trimmed();
     } else {
         result.success = false;
         result.error = "不支持的数组运算符";
         return result;
     }

     // 解析标量值（可能包含变量）
     ExpressionResult scalarResult = parseExpression(scalarStr);
     if (!scalarResult.success) {
         return scalarResult;
     }

     // 转换为数值
     bool ok = false;
     double scalar = scalarResult.value.toDouble(&ok);
     if (!ok) {
         result.success = false;
         result.error = "标量值不是有效的数值";
         return result;
     }

     // 执行数组运算
     result.value = performArrayOperation(arrayStr, QString("%1%2").arg(op).arg(scalar));
     result.success = true;

     return result;
}
