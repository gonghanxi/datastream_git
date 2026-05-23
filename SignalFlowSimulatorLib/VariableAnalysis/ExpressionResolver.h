// ExpressionResolver.h
#ifndef EXPRESSIONRESOLVER_H
#define EXPRESSIONRESOLVER_H

#include <QString>
#include <QMap>
#include "MathExpressionCalculator.h"
#include "VariableScopeManager.h"

struct ResolutionResult {
    QString expression;     // 原始表达式
    QString value;         // 解析后的值
    bool success;          // 是否成功
    QString error;         // 错误信息

    ResolutionResult() : success(false) {}
    ResolutionResult(const QString& v, bool s) : value(v), success(s) {}
};
//表达式解析器
class ExpressionResolver {
public:
    ExpressionResolver(VariableScopeManager* scopeMgr);

    // 主解析接口
    ResolutionResult resolveExpression(
            const QString& scopeId,
            const QString& expression,
            const QString& calculateValue = QString(),
            bool skipCalculateValue = false);

    // 批量解析
    QMap<QString, ResolutionResult> resolveExpressions(
        const QString& scopeId,
        const QMap<QString, QString>& expressions);

    // 带上下文的解析
    ResolutionResult resolveWithContext(
        const QString& scopeId,
        const QString& expression,
        const QMap<QString, QString>& localContext);

private:
    // 解析步骤
    ResolutionResult resolveStep1_CheckCalculateValue(
        const QString& expression,
        const QString& calculateValue);

    ResolutionResult resolveStep2_ReplaceVariables(
        const QString& scopeId,
        const QString& expression);

    ResolutionResult resolveStep3_EvaluateMath(
        const QString& expression);

    ResolutionResult resolveStep4_ParseComplex(
        const QString& expression);

    ResolutionResult resolveStep5_ParseArray(
        const QString& expression);

    // 辅助方法
    bool isPureValue(const QString& str) const;
    bool containsVariableReference(const QString& str) const;

private:
    VariableScopeManager* m_scopeMgr;
};

#endif // EXPRESSIONRESOLVER_H
