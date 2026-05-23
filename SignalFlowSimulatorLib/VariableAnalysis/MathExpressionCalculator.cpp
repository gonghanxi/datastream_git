#include "MathExpressionCalculator.h"
#include <QRegularExpression>
#include <QDebug>

//静态变量初始化
QMap<QString, double> MathExpressionCalculator::m_constants;
QMap<QString, std::function<double(double)>> MathExpressionCalculator::m_functions;
bool MathExpressionCalculator::m_initialized = false;
MathExpressionCalculator::MathExpressionCalculator()
{
    initialize();
}

bool MathExpressionCalculator::isPureNumber(const QString &str)
{
    QString trimmed = str.trimmed();
    bool ok = false;
    trimmed.toDouble(&ok);
    return ok;
}

bool MathExpressionCalculator::isMathExpression(const QString &expr)
{
    QString trimmed = expr.trimmed();

    // 空字符串不是表达式
    if (trimmed.isEmpty()) return false;

    // 纯数字不是表达式（已经是值）
    if (isPureNumber(trimmed)) return false;

    // 包含运算符
    if (trimmed.contains(QRegularExpression("[+\\-*/^]"))) return true;

    // 包含函数调用
    if (trimmed.contains(QRegularExpression("[a-zA-Z_]+\\(.*\\)"))) return true;

    // 包含内置常量（大小写不敏感）
    QString lower = trimmed.toLower();
    if (m_constants.contains(lower)) return true;

    return false;
}

double MathExpressionCalculator::evaluate(const QString &expr, bool &ok)
{
    initialize();
    return parseArithmeticExpression(expr, ok);
}

bool MathExpressionCalculator::isComplex(const QString& expr)
{
    QString trimmed = expr.trimmed();

    // 空字符串不是复数
    if (trimmed.isEmpty()) return false;

    // 格式1: (a, b)
    if (trimmed.startsWith('(') && trimmed.endsWith(')') && trimmed.contains(',')) {
        return true;
    }

    // 格式2: a + bi, a + bj, a + b*i, a + b*j, a + i*b, a + j*b, a + i*b, a + j*b
    static QRegularExpression complexRegex(
        "^([+-]?\\d*\\.?\\d*)?\\s*([+-])\\s*"           // 实部和符号
        "(?:"                                           // 开始可选组
            "\\d*\\.?\\d*\\s*\\*?\\s*[ij]|"            // 格式: b*i 或 bi (系数在前)
            "[ij]\\s*\\*?\\s*\\d*\\.?\\d*"              // 格式: i*b (单位在前)
        ")$"
    );
    if (complexRegex.match(trimmed).hasMatch()) {
        return true;
    }

    // 格式3: 纯虚数 bi, bj, b*i, b*j, i, j, i*b, j*b
    static QRegularExpression imagOnlyRegex(
        "^([+-]?\\d*\\.?\\d*)?\\s*\\*?\\s*[ij]$|"       // 格式: bi, b*i, i, j
        "^[ij]\\s*\\*?\\s*\\d*\\.?\\d*$"                 // 格式: i*b, j*b
    );
    if (imagOnlyRegex.match(trimmed).hasMatch()) {
        return true;
    }

    return false;
}

std::pair<double, double> MathExpressionCalculator::parseComplex(const QString& expr)
{
    QString trimmed = expr.trimmed();
    double real = 0.0, imag = 0.0;

    qDebug() << "parseComplex 输入:" << trimmed;

    // 格式1: (a, b)
    QRegularExpression parenRegex("^\\(\\s*([^,]+)\\s*,\\s*([^)]+)\\s*\\)$");
    QRegularExpressionMatch match = parenRegex.match(trimmed);
    if (match.hasMatch()) {
        bool ok1 = false, ok2 = false;
        real = match.captured(1).trimmed().toDouble(&ok1);
        imag = match.captured(2).trimmed().toDouble(&ok2);
        if (ok1 && ok2) {
            qDebug() << "括号格式复数解析成功:" << trimmed << "→" << real << "," << imag;
            return {real, imag};
        }
    }

    // 格式2: a + bi, a + bj, a + b*i, a + b*j (系数在前)
    QRegularExpression complexRegex1(
        "^([+-]?\\d*\\.?\\d*)?\\s*"           // 实部（可选）
        "([+-])"                               // 运算符
        "\\s*(\\d*\\.?\\d*)\\s*"               // 虚部系数
        "\\*?\\s*[ij]$"                        // 可选的*号和虚数单位
    );

    QRegularExpressionMatch match1 = complexRegex1.match(trimmed);
    if (match1.hasMatch()) {
        QString realStr = match1.captured(1);
        QString sign = match1.captured(2);
        QString imagStr = match1.captured(3);

        // 处理实部
        if (realStr.isEmpty() || realStr == "+") {
            real = 0.0;
        } else if (realStr == "-") {
            real = -0.0;
        } else {
            bool ok = false;
            real = realStr.toDouble(&ok);
            if (!ok) real = 0.0;
        }

        // 处理虚部系数
        if (imagStr.isEmpty()) {
            imag = 1.0;  // "i" 或 "j" 的情况
        } else {
            bool ok = false;
            imag = imagStr.toDouble(&ok);
            if (!ok) imag = 1.0;
        }

        // 应用符号
        if (sign == "-") {
            imag = -imag;
        }

        qDebug() << "格式2复数解析成功(系数在前):" << trimmed
                 << "→ real:" << real << ", imag:" << imag;
        return {real, imag};
    }

    // 格式3: a + i*b, a + j*b, a + i*2, a + j*2 (单位在前)
    QRegularExpression complexRegex2(
        "^([+-]?\\d*\\.?\\d*)?\\s*"           // 实部（可选）
        "([+-])"                               // 运算符
        "\\s*[ij]\\s*\\*?\\s*"                  // 虚数单位和可选的*
        "(\\d*\\.?\\d*)$"                       // 虚部系数
    );

    QRegularExpressionMatch match2 = complexRegex2.match(trimmed);
    if (match2.hasMatch()) {
        QString realStr = match2.captured(1);
        QString sign = match2.captured(2);
        QString imagStr = match2.captured(3);

        // 处理实部
        if (realStr.isEmpty() || realStr == "+") {
            real = 0.0;
        } else if (realStr == "-") {
            real = -0.0;
        } else {
            bool ok = false;
            real = realStr.toDouble(&ok);
            if (!ok) real = 0.0;
        }

        // 处理虚部系数
        if (imagStr.isEmpty()) {
            imag = 1.0;
        } else {
            bool ok = false;
            imag = imagStr.toDouble(&ok);
            if (!ok) imag = 1.0;
        }

        // 应用符号
        if (sign == "-") {
            imag = -imag;
        }

        qDebug() << "格式3复数解析成功(单位在前):" << trimmed
                 << "→ real:" << real << ", imag:" << imag;
        return {real, imag};
    }

    // 格式4: 纯虚数 bi, bj, b*i, b*j (系数在前)
    QRegularExpression imagOnlyRegex1(
        "^([+-]?\\d*\\.?\\d*)?\\s*\\*?\\s*[ij]$"
    );
    QRegularExpressionMatch match3 = imagOnlyRegex1.match(trimmed);
    if (match3.hasMatch()) {
        QString imagStr = match3.captured(1);

        if (imagStr.isEmpty() || imagStr == "+") {
            imag = 1.0;
        } else if (imagStr == "-") {
            imag = -1.0;
        } else {
            bool ok = false;
            imag = imagStr.toDouble(&ok);
            if (!ok) imag = 1.0;
        }

        qDebug() << "纯虚数解析成功(系数在前):" << trimmed << "→ imag:" << imag;
        return {0.0, imag};
    }

    // 格式5: 纯虚数 i*b, j*b (单位在前)
    QRegularExpression imagOnlyRegex2(
        "^[ij]\\s*\\*?\\s*(\\d*\\.?\\d*)$"
    );
    QRegularExpressionMatch match4 = imagOnlyRegex2.match(trimmed);
    if (match4.hasMatch()) {
        QString imagStr = match4.captured(1);

        if (imagStr.isEmpty()) {
            imag = 1.0;
        } else {
            bool ok = false;
            imag = imagStr.toDouble(&ok);
            if (!ok) imag = 1.0;
        }

        qDebug() << "纯虚数解析成功(单位在前):" << trimmed << "→ imag:" << imag;
        return {0.0, imag};
    }

    qDebug() << "复数解析失败:" << trimmed;
    return {0.0, 0.0};
}

QString MathExpressionCalculator::formatComplex(double real, double imag)
{
    return QString("(%1, %2)").arg(real, 0, 'g', 15).arg(imag, 0, 'g', 15);
}

bool MathExpressionCalculator::isArray(const QString &expr)
{
    QString trimmed = expr.trimmed();
    return trimmed.startsWith('[') && trimmed.endsWith(']');
}

QStringList MathExpressionCalculator::parseArrayElements(const QString &arrayStr)
{
    QStringList elements;
    QString content = arrayStr.mid(1, arrayStr.length() - 2).trimmed();

    if (content.isEmpty()) return elements;

    int bracketDepth = 0;
    int parenDepth = 0;
    QString current;

    for (int i = 0; i < content.length(); i++) {
        QChar ch = content[i];

        if (ch == '[') bracketDepth++;
        else if (ch == ']') bracketDepth--;
        else if (ch == '(') parenDepth++;
        else if (ch == ')') parenDepth--;
        else if (ch == ',' && bracketDepth == 0 && parenDepth == 0) {
            if (!current.trimmed().isEmpty()) {
                elements.append(current.trimmed());
            }
            current.clear();
            continue;
        }

        current.append(ch);
    }

    if (!current.trimmed().isEmpty()) {
        elements.append(current.trimmed());
    }

    return elements;
}

QString MathExpressionCalculator::evaluateArray(const QString &expr, bool &ok)
{
    ok = true;

    if (!isArray(expr)) {
        ok = false;
        return expr;
    }

    QStringList elements = parseArrayElements(expr);
    QStringList resultElements;

    for (const QString& elem : elements) {
        if (isArray(elem)) {
            // 嵌套数组
            QString nestedResult = evaluateArray(elem, ok);
            if (!ok) return expr;
            resultElements.append(nestedResult);
        } else {
            // 普通元素
            double val = parseArithmeticExpression(elem, ok);
            if (!ok) return expr;
            resultElements.append(QString::number(val, 'g', 15));
        }
    }

    return "[" + resultElements.join(", ") + "]";
}

bool MathExpressionCalculator::containsLetters(const QString &str)
{
    for (const QChar& ch : str) {
        if (ch.isLetter()) return true;
    }
    return false;
}

bool MathExpressionCalculator::isBuiltInConstant(const QString &name)
{
    return m_constants.contains(name.toLower());
}

bool MathExpressionCalculator::isBuiltInFunction(const QString &name)
{
    return m_functions.contains(name.toLower());
}

int MathExpressionCalculator::getOperatorPriority(const QChar &op)
{
    switch (op.unicode()) {
        case '+': case '-': return 1;
        case '*': case '/': case '%': return 2;
        case '^': return 3;
        default: return 0;
    }
}

double MathExpressionCalculator::parseArithmeticExpression(const QString &expr, bool &ok)
{
    ok = true;
    QString expression = expr.trimmed();

    // 移除外层括号
    while (expression.startsWith('(') && expression.endsWith(')')) {
        expression = expression.mid(1, expression.length() - 2).trimmed();
    }

    // 检查是否是数值
    double numValue;
    if (isPureNumber(expression)) {
        return expression.toDouble(&ok);
    }

    // 检查常量（大小写不敏感）
    QString lowerExpr = expression.toLower();
    if (m_constants.contains(lowerExpr)) {
        return m_constants[lowerExpr];
    }

    // 检查函数
    QRegularExpression funcRegex("^([a-zA-Z_][a-zA-Z0-9_]*)\\(([^)]+)\\)$");
    QRegularExpressionMatch funcMatch = funcRegex.match(expression);
    if (funcMatch.hasMatch()) {
        QString funcName = funcMatch.captured(1);
        QString argsStr = funcMatch.captured(2);

        if (m_functions.contains(funcName.toLower())) {
            double arg = parseArithmeticExpression(argsStr, ok);
            if (ok) {
                return evaluateFunction(funcName, arg);
            }
        }
    }

    // 查找最低优先级的运算符
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
        if (!ok) return 0;

        double rightVal = parseArithmeticExpression(right, ok);
        if (!ok) return 0;

        return evaluateBinaryOp(leftVal, rightVal, lowestOp);
    }

    ok = false;
    return 0;
}

double MathExpressionCalculator::evaluateBinaryOp(double left, double right, QChar op)
{
    switch (op.unicode()) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': return right != 0 ? left / right : 0;
        case '%': return right != 0 ? std::fmod(left, right) : 0;
        case '^': return std::pow(left, right);
        default: return 0;
    }
}

double MathExpressionCalculator::evaluateFunction(const QString &funcName, double arg)
{
    QString lower = funcName.toLower();
    if (m_functions.contains(lower)) {
        return m_functions[lower](arg);
    }
    return 0;
}

void MathExpressionCalculator::initialize()
{
    if (m_initialized) return;

    // 常量
    m_constants["pi"] = M_PI;
    m_constants["e"] = M_E;
    m_constants["inf"] = std::numeric_limits<double>::infinity();
    m_constants["nan"] = std::numeric_limits<double>::quiet_NaN();

    // 三角函数
    m_functions["sin"] = [](double x) { return std::sin(x); };
    m_functions["cos"] = [](double x) { return std::cos(x); };
    m_functions["tan"] = [](double x) { return std::tan(x); };
    m_functions["asin"] = [](double x) { return std::asin(x); };
    m_functions["acos"] = [](double x) { return std::acos(x); };
    m_functions["atan"] = [](double x) { return std::atan(x); };
    m_functions["sinh"] = [](double x) { return std::sinh(x); };
    m_functions["cosh"] = [](double x) { return std::cosh(x); };
    m_functions["tanh"] = [](double x) { return std::tanh(x); };

    // 其他函数
    m_functions["exp"] = [](double x) { return std::exp(x); };
    m_functions["log"] = [](double x) { return std::log(x); };
    m_functions["log10"] = [](double x) { return std::log10(x); };
    m_functions["sqrt"] = [](double x) { return std::sqrt(x); };
    m_functions["abs"] = [](double x) { return std::abs(x); };
    m_functions["floor"] = [](double x) { return std::floor(x); };
    m_functions["ceil"] = [](double x) { return std::ceil(x); };
    m_functions["round"] = [](double x) { return std::round(x); };

    m_initialized = true;
}
