// ExpressionResolver.cpp
#include "ExpressionResolver.h"
#include <QRegularExpression>
#include <QDebug>

ExpressionResolver::ExpressionResolver(VariableScopeManager* scopeMgr)
    : m_scopeMgr(scopeMgr)
{
}

bool ExpressionResolver::isPureValue(const QString& str) const
{
    QString trimmed = str.trimmed();

    // 空值
    if (trimmed.isEmpty()) return true;

    // 纯数字（包括科学计数法）
    if (MathExpressionCalculator::isPureNumber(trimmed)) return true;

    // 布尔值
    QString lower = trimmed.toLower();
    if (lower == "true" || lower == "false") return true;

    // 字符串（带引号）
    if ((trimmed.startsWith('"') && trimmed.endsWith('"')) ||
        (trimmed.startsWith('\'') && trimmed.endsWith('\''))) {
        return true;
    }

    // 【新增】检查是否包含运算符，如果有运算符则不是纯值
    if (trimmed.contains('+') || trimmed.contains('-') ||
        trimmed.contains('*') || trimmed.contains('/') ||
        trimmed.contains('^') || trimmed.contains('(') ||
        trimmed.contains(')')) {
        return false;
    }

    // 检查是否是已解析的复数格式 (a, b)
    if (trimmed.startsWith('(') && trimmed.endsWith(')') && trimmed.contains(',')) {
        return true;
    }

    return false;
}

bool ExpressionResolver::containsVariableReference(const QString& str) const
{
    if (MathExpressionCalculator::isPureNumber(str)) return false;
    if (MathExpressionCalculator::isMathExpression(str)) return true;
    return MathExpressionCalculator::containsLetters(str);
}

ResolutionResult ExpressionResolver::resolveStep1_CheckCalculateValue(
    const QString& expression,
    const QString& calculateValue)
{
    ResolutionResult result;
    result.expression = expression;

    // 如果提供了calculateValue且不为空，优先使用
    if (!calculateValue.isEmpty()) {
        qDebug() << "使用calculateValue:" << expression << "→" << calculateValue;
        result.value = calculateValue;
        result.success = true;
        return result;
    }

    result.success = false;
    return result;
}

ResolutionResult ExpressionResolver::resolveStep2_ReplaceVariables(
    const QString& scopeId,
    const QString& expression)
{
    ResolutionResult result;
    result.expression = expression;

    // 如果已经是纯值，不需要替换
    if (isPureValue(expression)) {
        result.value = expression;
        result.success = true;
        return result;
    }

    QString resolved = expression;

    // 提取所有可能的变量引用
    static QRegularExpression varRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b(?![(])");
    QRegularExpressionMatchIterator it = varRegex.globalMatch(expression);

    QMap<QString, QString> replacements;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString varName = match.captured(1);

        // 跳过数学函数和常量
        if (MathExpressionCalculator::isBuiltInFunction(varName) ||
            MathExpressionCalculator::isBuiltInConstant(varName)) {
            continue;
        }

        // 跳过复数单位
        if (varName.toLower() == "i" || varName.toLower() == "j") {
            continue;
        }

        // 解析变量值
        if (!replacements.contains(varName)) {
            bool found = false;
            QString varValue = m_scopeMgr->resolveVariableValue(scopeId, varName, &found);
            if (found && !varValue.isEmpty()) {
                replacements[varName] = varValue;
                qDebug() << "变量替换:" << varName << "=" << varValue;
            }
        }
    }

    // 执行替换（按变量名长度降序，避免部分匹配）
    QList<QString> sortedVars = replacements.keys();
    std::sort(sortedVars.begin(), sortedVars.end(),
              [](const QString& a, const QString& b) { return a.length() > b.length(); });

    for (const QString& varName : sortedVars) {
        resolved.replace(QRegularExpression("\\b" + varName + "\\b"), replacements[varName]);
    }

    result.value = resolved;
    result.success = (resolved != expression); // 只要有替换就算成功
    qDebug() << "resolveStep2_ReplaceVariables - 替换是否成功：" << (result.success ? "true" : "false");

    return result;
}

ResolutionResult ExpressionResolver::resolveStep3_EvaluateMath(
    const QString& expression)
{
    ResolutionResult result;
    result.expression = expression;

    QString trimmed = expression.trimmed();

    // 检查是否是数学表达式
    if (MathExpressionCalculator::isMathExpression(trimmed)) {
        bool ok = false;
        double value = MathExpressionCalculator::evaluate(trimmed, ok);

        if (ok) {
            result.value = QString::number(value, 'g', 15);
            result.success = true;
            qDebug() << "数学表达式解析:" << trimmed << "=" << result.value;
            return result;
        }
    }

    result.success = false;
    return result;
}

ResolutionResult ExpressionResolver::resolveStep4_ParseComplex(
    const QString& expression)
{
    ResolutionResult result;
    result.expression = expression;

    QString trimmed = expression.trimmed();

    if (MathExpressionCalculator::isComplex(trimmed)) {
        auto complex = MathExpressionCalculator::parseComplex(trimmed);
        result.value = MathExpressionCalculator::formatComplex(complex.first, complex.second);
        result.success = true;
        qDebug() << "复数解析:" << trimmed << "=" << result.value;
        return result;
    }

    result.success = false;
    return result;
}

ResolutionResult ExpressionResolver::resolveStep5_ParseArray(
    const QString& expression)
{
    ResolutionResult result;
    result.expression = expression;

    QString trimmed = expression.trimmed();

    if (MathExpressionCalculator::isArray(trimmed)) {
        bool ok = false;
        QString arrayValue = MathExpressionCalculator::evaluateArray(trimmed, ok);

        if (ok) {
            result.value = arrayValue;
            result.success = true;
            qDebug() << "数组解析:" << trimmed << "=" << result.value;
            return result;
        }
    }

    result.success = false;
    return result;
}

ResolutionResult ExpressionResolver::resolveExpression(
    const QString& scopeId,
    const QString& expression,
    const QString& calculateValue,
    bool skipCalculateValue)
{
    ResolutionResult result;
    result.expression = expression;

    QString currentExpr = expression.trimmed();

    // 空表达式
    if (currentExpr.isEmpty()) {
        result.value = "";
        result.success = true;
        return result;
    }

    qDebug() << "解析表达式:" << currentExpr
             << "作用域:" << scopeId
             << "calculateValue: " << calculateValue
             << "跳过calculateValue:" << skipCalculateValue;

    // Step 1: 替换变量引用
    result = resolveStep2_ReplaceVariables(scopeId, currentExpr);
    if (result.success && result.value != currentExpr) {
        QString replacedExpr = result.value;

        // 检查替换后的表达式是否还需要继续解析
        // 判断是否是纯值（数值、布尔、字符串）或已经是最终结果
        bool isPureAfterReplace = isPureValue(replacedExpr);

        // 检查是否是数学表达式（包含运算符）
        bool isMathAfterReplace = MathExpressionCalculator::isMathExpression(replacedExpr);

        // 检查是否是数组或复数
        bool isArrayAfterReplace = MathExpressionCalculator::isArray(replacedExpr);
        bool isComplexAfterReplace = MathExpressionCalculator::isComplex(replacedExpr);

        qDebug() << "变量替换后: " << replacedExpr
                 << "| 纯值:" << isPureAfterReplace
                 << "| 数学表达式:" << isMathAfterReplace
                 << "| 数组:" << isArrayAfterReplace
                 << "| 复数:" << isComplexAfterReplace;

        if (isPureAfterReplace) {
            // 纯数值，直接返回，不需要递归
            result.value = replacedExpr;
            result.success = true;
            qDebug() << "变量替换后得到纯值，直接返回:" << replacedExpr;
            return result;
        } else if (isMathAfterReplace || isArrayAfterReplace || isComplexAfterReplace) {
            // 替换后仍是表达式，递归解析，但跳过 calculateValue
            // 因为 calculateValue 是针对原始表达式的，不适合替换后的表达式
            qDebug() << "变量替换后仍是表达式，递归解析（跳过calculateValue）";
            return resolveExpression(scopeId, replacedExpr, calculateValue, false);
        } else {
            // 其他情况（如字符串），直接返回
            result.value = replacedExpr;
            result.success = true;
            qDebug() << "变量替换后得到非表达式，直接返回:" << replacedExpr;
            return result;
        }
    }

    // Step 2: 优先使用calculateValue（但只在 skipCalculateValue == false 时）
    if (!skipCalculateValue) {
        result = resolveStep1_CheckCalculateValue(currentExpr, calculateValue);
        if (result.success) {
            qDebug() << "使用calculateValue成功:" << calculateValue;
            return result;
        }
    }

    // Step 3: 解析数学表达式
    result = resolveStep3_EvaluateMath(currentExpr);
    if (result.success) {
        qDebug() << "数学表达式解析成功:" << currentExpr << "=" << result.value;
        return result;
    }

    // Step 4: 解析复数
    result = resolveStep4_ParseComplex(currentExpr);
    if (result.success) {
        qDebug() << "复数解析成功:" << currentExpr << "=" << result.value;
        return result;
    }

    // Step 5: 解析数组
    result = resolveStep5_ParseArray(currentExpr);
    if (result.success) {
        qDebug() << "数组解析成功:" << currentExpr << "=" << result.value;
        return result;
    }

    // 默认返回原值
    result.value = currentExpr;
    result.success = true;
    result.error = "未找到匹配的解析规则，使用原始值";

    qDebug() << "未找到匹配的解析规则:" << currentExpr << "→" << result.value;
    return result;
}

QMap<QString, ResolutionResult> ExpressionResolver::resolveExpressions(
    const QString& scopeId,
    const QMap<QString, QString>& expressions)
{
    QMap<QString, ResolutionResult> results;

    for (auto it = expressions.begin(); it != expressions.end(); ++it) {
        results[it.key()] = resolveExpression(scopeId, it.value());
    }

    return results;
}

ResolutionResult ExpressionResolver::resolveWithContext(
    const QString& scopeId,
    const QString& expression,
    const QMap<QString, QString>& localContext)
{
    // 先应用本地上下文
    QString expr = expression;

    for (auto it = localContext.begin(); it != localContext.end(); ++it) {
        expr.replace(QRegularExpression("\\b" + it.key() + "\\b"), it.value());
    }

    // 再正常解析
    return resolveExpression(scopeId, expr);
}
