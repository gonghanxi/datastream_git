#ifndef VAREXPRESSIONPARSE_H
#define VAREXPRESSIONPARSE_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QRegularExpression>
#include <cmath>
#include "VariableAnalysis/Variable.h"

// 变量解析和替换结果
struct ExpressionResult {
    QString expression;     // 原始表达式
    QString value;          // 计算/替换后的值
    bool success;           // 是否成功
    QString error;          // 错误信息
};

class VarExpressionParse
{
public:
    VarExpressionParse();
    ~VarExpressionParse();

    // 设置变量表
    void setVariables(const QVector<Variable>& variables);
    void setVariables(const QJsonArray& varsArray);

    // 解析表达式中的变量引用
    ExpressionResult parseExpression(const QString& expression);

    // 解析参数值，替换其中的变量引用
    QString parseParameterValue(const QString& paramValue);

    // 获取所有变量
    QMap<QString, Variable> getAllVariables() const;

    // 获取指定上下文的变量（用于子系统）
    QMap<QString, Variable> getContextVariables(const QString& contextId = "") const;

    // 清空变量表
    void clear();

    // 解析数组表达式
    ExpressionResult parseArrayExpression(const QString& expression);
    ExpressionResult parseArrayWithExpressions(const QString &expression);
    ExpressionResult parseOneDimensionalArray(const QString &content);
    ExpressionResult parseMatrixExpression(const QString &content);
    QStringList splitArrayContent(const QString &content);
    // 判断是否是数组表达式（支持内部包含表达式）
    bool isArrayExpression(const QString &expression);

private:
    // 变量表：key为变量名，value为变量信息
    QMap<QString, Variable> m_variables;

    // 按ID索引的变量（用于子系统上下文）
    QMap<QString, QMap<QString, Variable>> m_contextVariables;

    // 数学函数映射
    QMap<QString, std::function<double(double)>> m_mathFunctions;

    // 常量映射
    QMap<QString, double> m_constants;

    // 初始化数学函数和常量
    void initializeMathFunctions();

    // 提取表达式中的变量名
    QStringList extractVariableNames(const QString& expression);
    // 替换表达式中的变量引用
    QString replaceVariables(const QString& expression,
                            const QMap<QString, QString>& varValues);

    // 计算数学表达式
    ExpressionResult evaluateMathExpression(const QString& expression);

    // 检查变量名是否有效
    bool isValidVariableName(const QString& name);
    // 解析变量数据类型
    QVariant parseVariableValue(const Variable& var);
    // 将变量值转换为字符串
    QString variableValueToString(const QVariant& value, const QString& dataType);

    // 安全转换字符串到数值
    bool safeToDouble(const QString& str, double& result);
    bool safeToInt(const QString& str, int& result);

    // 优先级判断
    int getOperatorPriority(const QChar& op);

    // 递归解析表达式
    double parseArithmeticExpression(const QString& expr, bool& ok);
    double evaluateBinaryOp(double left, double right, QChar op);

    // 执行数组运算
    QString performArrayOperation(const QString& arrayStr, const QString& operation);

    // 解析数组和标量的运算
    ExpressionResult parseArrayScalarOperation(const QString& expression);

    // 特殊函数处理
    double evaluateFunction(const QString& funcName, double arg);
    double evaluateFunction(const QString& funcName, double arg1, double arg2);
};

#endif // VAREXPRESSIONPARSE_H
