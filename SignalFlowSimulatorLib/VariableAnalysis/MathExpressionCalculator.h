#ifndef MATHEXPRESSIONCALCULATOR_H
#define MATHEXPRESSIONCALCULATOR_H


#include <QString>
#include <QMap>
#include <functional>
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

//数学表达式计算器
class MathExpressionCalculator {
public:
    MathExpressionCalculator();

    // 判断是否是纯数字
    static bool isPureNumber(const QString& str);

    // 判断是否是数学表达式
    static bool isMathExpression(const QString& expr);

    // 计算数学表达式
    static double evaluate(const QString& expr, bool& ok);

    // 复数解析
    static bool isComplex(const QString& expr);
    static std::pair<double, double> parseComplex(const QString& expr);
    static QString formatComplex(double real, double imag);

    // 数组解析
    static bool isArray(const QString& expr);
    static QStringList parseArrayElements(const QString& arrayStr);
    static QString evaluateArray(const QString& expr, bool& ok);

    // 判断是否包含字母（用于区分变量）
    static bool containsLetters(const QString& str);

    // 判断是否是内置常量/函数
    static bool isBuiltInConstant(const QString& name);
    static bool isBuiltInFunction(const QString& name);

private:
    // 运算符优先级
    static int getOperatorPriority(const QChar& op);

    // 递归解析算术表达式
    static double parseArithmeticExpression(const QString& expr, bool& ok);

    // 二元运算
    static double evaluateBinaryOp(double left, double right, QChar op);

    // 函数运算
    static double evaluateFunction(const QString& funcName, double arg);

private:
    static QMap<QString, double> m_constants;
    static QMap<QString, std::function<double(double)>> m_functions;
    static bool m_initialized;

    static void initialize();
};

#endif // MATHEXPRESSIONCALCULATOR_H
