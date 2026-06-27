#include "MATLAB_Script_Block.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <exception>

#include "octave/dMatrix.h"
#include "octave/CMatrix.h"

namespace SV = SystemVueModelBuilder;

// 静态成员变量初始化
octave::interpreter* MATLAB_Script_Block::s_sharedInterp = nullptr;
int MATLAB_Script_Block::s_instanceCount = 0;

// ===================== Octave 数据转换辅助方法 =====================

octave_value MATLAB_Script_Block::vectorToOctave(const std::vector<double>& data)
{
    ::Matrix mat(static_cast<octave_idx_type>(data.size()), 1);
    for (size_t i = 0; i < data.size(); i++) {
        mat(static_cast<octave_idx_type>(i), 0) = data[i];
    }
    return octave_value(mat);
}

octave_value MATLAB_Script_Block::vectorToOctave(const std::vector<std::complex<double>>& data)
{
    ComplexMatrix mat(static_cast<octave_idx_type>(data.size()), 1);
    for (size_t i = 0; i < data.size(); i++) {
        mat(static_cast<octave_idx_type>(i), 0) = Complex(data[i].real(), data[i].imag());
    }
    return octave_value(mat);
}

octave_value MATLAB_Script_Block::matrixToOctave(const std::vector<SV::DoubleMatrix>& data)
{
    if (data.size() == 1) {
        // 单个矩阵，直接返回 Matrix
        SV::Matrix<double> matVue = data[0];
        int rows = matVue.NumRows();
        int cols = matVue.NumColumns();
        ::Matrix mat(rows, cols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                mat(r, c) = matVue(r, c);
            }
        }
        return octave_value(mat);
    }
    // 多个矩阵，使用 Cell 数组
    Cell cell(static_cast<octave_idx_type>(data.size()), 1);
    for (size_t i = 0; i < data.size(); i++) {
        SV::Matrix<double> matVue = data[i];
        int rows = matVue.NumRows();
        int cols = matVue.NumColumns();
        ::Matrix mat(rows, cols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                mat(r, c) = matVue(r, c);
            }
        }
        cell(static_cast<octave_idx_type>(i), 0) = octave_value(mat);
    }
    return octave_value(cell);
}

octave_value MATLAB_Script_Block::complexMatrixToOctave(const std::vector<SV::DComplexMatrix>& data)
{
    if (data.size() == 1) {
        SV::DComplexMatrix matVue = data[0];
        int rows = matVue.NumRows();
        int cols = matVue.NumColumns();
        ComplexMatrix mat(rows, cols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                mat(r, c) = Complex(matVue(r, c).real(), matVue(r, c).imag());
            }
        }
        return octave_value(mat);
    }
    Cell cell(static_cast<octave_idx_type>(data.size()), 1);
    for (size_t i = 0; i < data.size(); i++) {
        SV::DComplexMatrix matVue = data[i];
        int rows = matVue.NumRows();
        int cols = matVue.NumColumns();
        ComplexMatrix mat(rows, cols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                mat(r, c) = Complex(matVue(r, c).real(), matVue(r, c).imag());
            }
        }
        cell(static_cast<octave_idx_type>(i), 0) = octave_value(mat);
    }
    return octave_value(cell);
}

// ===================== 参数解析辅助方法 =====================

bool MATLAB_Script_Block::isComplexElement(const QString& str)
{
    QString trimmed = str.trimmed();
    return trimmed.startsWith('(') && trimmed.endsWith(')');
}

std::complex<double> MATLAB_Script_Block::parseComplexElement(const QString& str)
{
    QString trimmed = str.trimmed();
    // 去掉外层括号 (real,imag)
    if (trimmed.startsWith('(') && trimmed.endsWith(')')) {
        trimmed = trimmed.mid(1, trimmed.length() - 2).trimmed();
    }
    int commaPos = trimmed.indexOf(',');
    if (commaPos >= 0) {
        double real = trimmed.left(commaPos).trimmed().toDouble();
        double imag = trimmed.mid(commaPos + 1).trimmed().toDouble();
        return std::complex<double>(real, imag);
    }
    // 只有实部
    return std::complex<double>(trimmed.toDouble(), 0.0);
}

void MATLAB_Script_Block::assignArrayParam(const std::string& name, const QString& innerStr)
{
    // 按逗号分割元素（需考虑括号内的逗号）
    QStringList elements;
    int depth = 0;
    QString current;
    for (int i = 0; i < innerStr.length(); ++i) {
        QChar ch = innerStr[i];
        if (ch == '(') depth++;
        else if (ch == ')') depth--;
        if (ch == ',' && depth == 0) {
            elements.append(current.trimmed());
            current.clear();
        } else {
            current.append(ch);
        }
    }
    if (!current.trimmed().isEmpty()) {
        elements.append(current.trimmed());
    }

    if (elements.isEmpty()) return;

    // 判断是否为复数数组
    bool hasComplex = false;
    for (const QString& elem : elements) {
        if (isComplexElement(elem)) {
            hasComplex = true;
            break;
        }
    }

    if (hasComplex) {
        ComplexMatrix mat(1, static_cast<octave_idx_type>(elements.size()));
        for (int i = 0; i < elements.size(); ++i) {
            std::complex<double> val = parseComplexElement(elements[i]);
            mat(0, static_cast<octave_idx_type>(i)) = Complex(val.real(), val.imag());
        }
        m_interp->get_evaluator().top_level_assign(name, octave_value(mat));
    } else {
        ::Matrix mat(1, static_cast<octave_idx_type>(elements.size()));
        for (int i = 0; i < elements.size(); ++i) {
            mat(0, static_cast<octave_idx_type>(i)) = elements[i].toDouble();
        }
        m_interp->get_evaluator().top_level_assign(name, octave_value(mat));
    }
}

void MATLAB_Script_Block::assignMatrixParam(const std::string& name, const QString& innerStr)
{
    // 按分号分割行
    QStringList rows = innerStr.split(';', Qt::SkipEmptyParts);
    if (rows.isEmpty()) return;

    // 解析每行的元素（考虑括号内的逗号和分号）
    std::vector<QStringList> matrixRows;
    int maxCols = 0;
    bool hasComplex = false;

    for (const QString& row : rows) {
        QStringList elements;
        int depth = 0;
        QString current;
        for (int i = 0; i < row.length(); ++i) {
            QChar ch = row[i];
            if (ch == '(') depth++;
            else if (ch == ')') depth--;
            if (ch == ',' && depth == 0) {
                elements.append(current.trimmed());
                current.clear();
            } else {
                current.append(ch);
            }
        }
        if (!current.trimmed().isEmpty()) {
            elements.append(current.trimmed());
        }
        if (elements.size() > maxCols) maxCols = elements.size();
        for (const QString& elem : elements) {
            if (isComplexElement(elem)) hasComplex = true;
        }
        matrixRows.push_back(elements);
    }

    int numRows = static_cast<int>(matrixRows.size());

    if (hasComplex) {
        ComplexMatrix mat(static_cast<octave_idx_type>(numRows), static_cast<octave_idx_type>(maxCols));
        for (int r = 0; r < numRows; ++r) {
            const QStringList& cols = matrixRows[r];
            for (int c = 0; c < cols.size(); ++c) {
                std::complex<double> val = parseComplexElement(cols[c]);
                mat(static_cast<octave_idx_type>(r), static_cast<octave_idx_type>(c)) = Complex(val.real(), val.imag());
            }
        }
        m_interp->get_evaluator().top_level_assign(name, octave_value(mat));
    } else {
        ::Matrix mat(static_cast<octave_idx_type>(numRows), static_cast<octave_idx_type>(maxCols));
        for (int r = 0; r < numRows; ++r) {
            const QStringList& cols = matrixRows[r];
            for (int c = 0; c < cols.size(); ++c) {
                mat(static_cast<octave_idx_type>(r), static_cast<octave_idx_type>(c)) = cols[c].toDouble();
            }
        }
        m_interp->get_evaluator().top_level_assign(name, octave_value(mat));
    }
}

void MATLAB_Script_Block::assignComplexScalarParam(const std::string& name, const QString& str)
{
    QString trimmed = str.trimmed();
    double real = 0.0, imag = 0.0;

    if (trimmed.startsWith('(') && trimmed.endsWith(')')) {
        // 格式: (real,imag)
        QString inner = trimmed.mid(1, trimmed.length() - 2).trimmed();
        int commaPos = inner.indexOf(',');
        if (commaPos >= 0) {
            real = inner.left(commaPos).trimmed().toDouble();
            imag = inner.mid(commaPos + 1).trimmed().toDouble();
        } else {
            real = inner.toDouble();
        }
    } else {
        // 格式: real,imag
        int commaPos = trimmed.lastIndexOf(',');
        if (commaPos >= 0) {
            real = trimmed.left(commaPos).trimmed().toDouble();
            imag = trimmed.mid(commaPos + 1).trimmed().toDouble();
        }
    }

    m_interp->get_evaluator().top_level_assign(name, octave_value(std::complex<double>(real, imag)));
}

// ===================== 构造 / 析构 =====================

MATLAB_Script_Block::MATLAB_Script_Block(const std::string& name)
    : Block(name)
{
    // 使用共享 Octave 解释器（所有实例共用）
    if (s_instanceCount == 0) {
        s_sharedInterp = new octave::interpreter();
        s_sharedInterp->execute();
    }
    s_instanceCount++;
    m_interp = s_sharedInterp;  // 指向共享解释器
}

MATLAB_Script_Block::~MATLAB_Script_Block()
{
    s_instanceCount--;
    if (s_instanceCount == 0 && s_sharedInterp) {
        delete s_sharedInterp;
        s_sharedInterp = nullptr;
    }
}

bool MATLAB_Script_Block::Setup()
{
    Block::Setup();
    return true;
}

// ===================== Run =====================

bool MATLAB_Script_Block::Run()
{
    qDebug()<<"MATLAB_Script_Block Run start";
    if (!m_interp)
        return false;

    QMap<int, SV::PortMsg> ports = Block::getPortsMsg();

    // 收集输入数据并注入到 Octave 工作区
    QMap<QString, octave_value> inputs;
    for (auto port : ports) {
        if (port.putType == "in") {
            QString portName = QString("%1_%2").arg(getInstanceName().c_str()).arg(port.name);

            if (port.dataType == SV::PortMsg::REAL || port.dataType == SV::PortMsg::INT || port.dataType == SV::PortMsg::ANYTYPE) {
                auto inputPortData = ReadInputData<double>(port.name.toStdString());
                if (!inputPortData.empty()) {
                    inputs[port.name] = vectorToOctave(inputPortData);
                    qDebug() << QString("MATLAB_Script_Block Run set input double data[%1]:%2").arg(port.name).arg(inputPortData.size());
                }
            } else if (port.dataType == SV::PortMsg::COMPLEX) {
                auto inputPortData = ReadInputData<std::complex<double>>(port.name.toStdString());
                if (!inputPortData.empty()) {
                    inputs[port.name] = vectorToOctave(inputPortData);
                    qDebug() << QString("MATLAB_Script_Block Run set input complex data[%1]:%2").arg(port.name).arg(inputPortData.size());
                }
            } else if (port.dataType == SV::PortMsg::INT_MATRIX || port.dataType == SV::PortMsg::REAL_MATRIX) {
                auto inputPortData = ReadInputData<SV::DoubleMatrix>(port.name.toStdString());
                if (!inputPortData.empty()) {
                    inputs[port.name] = matrixToOctave(inputPortData);
                    qDebug() << QString("MATLAB_Script_Block Run set input DoubleMatrix data[%1]:%2").arg(port.name).arg(inputPortData.size());
                }
            } else if (port.dataType == SV::PortMsg::COMPLEX_MATRIX) {
                auto inputPortData = ReadInputData<SV::DComplexMatrix>(port.name.toStdString());
                if (!inputPortData.empty()) {
                    inputs[port.name] = complexMatrixToOctave(inputPortData);
                    qDebug() << QString("MATLAB_Script_Block Run set input DComplexMatrix data[%1]:%2").arg(port.name).arg(inputPortData.size());
                }
            }
        } else if (port.putType == "out") {
            // 输出端口在 Run 后半段处理
        }
    }

    try {
    // 添加脚本路径
    QString appPath = QCoreApplication::applicationDirPath();
    QString folderPath = appPath + "/m";
    m_interp->eval(QString("addpath('%1');").arg(folderPath).toStdString(), 0);
    qDebug() << QString("MATLAB_Script_Block Run2");

    // 重新注入当前模型的参数（多实例共享解释器时，确保参数正确）
    std::map<std::string, SV::Parameter> allparameters = getAllParameter();
    for (auto e : allparameters) {
        std::string Name = e.second.Name;
        if (Name != "Equations" && Name != "isUserDefined") {
            QString str = e.second.Value.c_str();
            QString trimmed = str.trimmed();
            if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                QString inner = trimmed.mid(1, trimmed.length() - 2).trimmed();
                if (inner.isEmpty()) {
                    m_interp->get_evaluator().top_level_assign(Name, octave_value(::Matrix(0, 0)));
                } else if (inner.contains(";")) {
                    assignMatrixParam(Name, inner);
                } else {
                    assignArrayParam(Name, inner);
                }
            } else if (trimmed.contains("(") || trimmed.contains(",")) {
                assignComplexScalarParam(Name, trimmed);
            } else {
                double value = std::stod(e.second.Value);
                m_interp->get_evaluator().top_level_assign(Name, octave_value(value));
            }
        }
    }

    // 将输入变量注入 Octave 工作区
    for (auto key : inputs.keys()) {
        m_interp->get_evaluator().top_level_assign(key.toStdString(), inputs[key]);
    }

    // 检查所有输入端口是否都有数据
    int expectedInputs = 0;
    for (auto port : ports) {
        if (port.putType == "in") expectedInputs++;
    }
    if (inputs.size() < expectedInputs) {
        return true;
    }

    // 执行生成的 MATLAB/Octave 函数
    m_interp->eval(QString("%1;").arg(callStr).toStdString(), 0);
    qDebug() << QString("MATLAB_Script_Block Run4");
    // 读取输出变量
    for (auto port : ports) {
        qDebug() << QString("MATLAB_Script_Block Run5");
        if (port.putType == "in") {
            continue;
        }
        else if (port.putType == "out") {

            qDebug() << QString("MATLAB_Script_Block Run get out value:%1").arg(port.name);
            octave_value outputVal = m_interp->get_evaluator().top_level_varval(port.name.toStdString());
            if (outputVal.is_defined()) {
                if (port.dataType == SV::PortMsg::REAL || port.dataType == SV::PortMsg::INT || port.dataType == SV::PortMsg::ANYTYPE) {
                    ::Matrix resultMat = outputVal.matrix_value();
                    int resultDataSize = resultMat.numel();
                    std::vector<double> outputData;
                    outputData.reserve(resultDataSize);
                    for (int i = 0; i < resultDataSize; ++i) {
                        outputData.push_back(resultMat.elem(i));
                    }
                    qDebug() << QString("MATLAB_Script_Block Run get output double data[%1]:%2").arg(port.name).arg(outputData.size());
                    WriteOutputData(port.name.toStdString().c_str(), outputData);

                } else if (port.dataType == SV::PortMsg::COMPLEX) {
                    ComplexMatrix resultMat = outputVal.complex_matrix_value();
                    int resultDataSize = resultMat.numel();
                    std::vector<std::complex<double>> outputData;
                    outputData.reserve(resultDataSize);
                    for (int i = 0; i < resultDataSize; ++i) {
                        Complex c = resultMat.elem(i);
                        outputData.push_back({c.real(), c.imag()});
                    }
                    qDebug() << QString("MATLAB_Script_Block Run get output complex data[%1]:%2").arg(port.name).arg(outputData.size());
                    WriteOutputData(port.name.toStdString().c_str(), outputData);

                } else if (port.dataType == SV::PortMsg::INT_MATRIX || port.dataType == SV::PortMsg::REAL_MATRIX) {
                    if (outputVal.iscell()) {
                        // Cell 数组：逐个提取矩阵
                        Cell cellArr = outputVal.cell_value();
                        int cellCount = cellArr.numel();
                        std::vector<double> outputData;
                        for (int ci = 0; ci < cellCount; ci++) {
                            ::Matrix subMat = cellArr.elem(ci).matrix_value();
                            int subSize = subMat.numel();
                            for (int i = 0; i < subSize; i++) {
                                outputData.push_back(subMat.elem(i));
                            }
                        }
                        qDebug() << QString("MATLAB_Script_Block Run get output REAL_MATRIX cell data[%1]:%2").arg(port.name).arg(outputData.size());
                        WriteOutputData(port.name.toStdString().c_str(), outputData);
                    } else {
                        // 普通矩阵
                        ::Matrix resultMat = outputVal.matrix_value();
                        int resultDataSize = resultMat.numel();
                        std::vector<double> outputData;
                        outputData.reserve(resultDataSize);
                        for (int i = 0; i < resultDataSize; ++i) {
                            outputData.push_back(resultMat.elem(i));
                        }
                        qDebug() << QString("MATLAB_Script_Block Run get output REAL_MATRIX data[%1]:%2").arg(port.name).arg(outputData.size());
                        WriteOutputData(port.name.toStdString().c_str(), outputData);
                    }

                } else if (port.dataType == SV::PortMsg::COMPLEX_MATRIX) {
                    if (outputVal.iscell()) {
                        Cell cellArr = outputVal.cell_value();
                        int cellCount = cellArr.numel();
                        std::vector<std::complex<double>> outputData;
                        for (int ci = 0; ci < cellCount; ci++) {
                            ComplexMatrix subMat = cellArr.elem(ci).complex_matrix_value();
                            int subSize = subMat.numel();
                            for (int i = 0; i < subSize; i++) {
                                Complex c = subMat.elem(i);
                                outputData.push_back({c.real(), c.imag()});
                            }
                        }
                        qDebug() << QString("MATLAB_Script_Block Run get output COMPLEX_MATRIX cell data[%1]:%2").arg(port.name).arg(outputData.size());
                        WriteOutputData(port.name.toStdString().c_str(), outputData);
                    } else {
                        ComplexMatrix resultMat = outputVal.complex_matrix_value();
                        int resultDataSize = resultMat.numel();
                        std::vector<std::complex<double>> outputData;
                        outputData.reserve(resultDataSize);
                        for (int i = 0; i < resultDataSize; ++i) {
                            Complex c = resultMat.elem(i);
                            outputData.push_back({c.real(), c.imag()});
                        }
                        qDebug() << QString("MATLAB_Script_Block Run get output COMPLEX_MATRIX data[%1]:%2").arg(port.name).arg(outputData.size());
                        WriteOutputData(port.name.toStdString().c_str(), outputData);
                    }
                }
            }
        }
    }
    } catch (const std::exception& e) {
        static bool logged = false;
        if (!logged) {
            LOG_ERROR("MATLAB_Script_Block Octave exception: ", e.what(),
                      " blockName: ", getInstanceName().c_str());
            std::string isUserDefined = getParameter("isUserDefined").Value;
            if (isUserDefined == "true") {
                LOG_ERROR("自定义模型语法/算法逻辑校验失败");
            }
            logged = true;
        }
        return false;
    } catch (...) {
        static bool logged = false;
        if (!logged) {
            LOG_ERROR("MATLAB_Script_Block Octave unknown exception, blockName: ",
                      getInstanceName().c_str());
            std::string isUserDefined = getParameter("isUserDefined").Value;
            if (isUserDefined == "true") {
                LOG_ERROR("自定义模型语法/算法逻辑校验失败");
            }
            logged = true;
        }
        return false;
    }
    // 自定义模型校验成功日志（只输出一次）
    {
        static bool loggedSuccess = false;
        if (!loggedSuccess) {
            std::string isUserDefined = getParameter("isUserDefined").Value;
            if (isUserDefined == "true") {
                LOG_INFO("自定义模型校验成功");
            }
            loggedSuccess = true;
        }
    }
    return true;
}
// ===================== Initialize =====================

bool MATLAB_Script_Block::Initialize()
{
    qDebug() << "MATLAB_Script_Block Initialize start";
    QString appPath = QCoreApplication::applicationDirPath();
    QString folderPath = appPath + "/m";
    QDir dir(folderPath);
    if (!dir.exists()) {
        dir.mkdir(folderPath);
    }
    qDebug() << "appPath: " << appPath;
    std::string Equations = getParameter("Equations").Value;

    if (!Equations.empty()) {
        QMap<int, SV::PortMsg> ports = getPortsMsg();
        qDebug() << "MATLAB_Script_Block Initialize ports size: " << ports.size();
        QStringList inputs;
        QStringList outputs;
        int i = 0;
        int j = 0;
        for (auto port : ports) {
            qDebug() << "MATLAB_Script_Block Initialize dataType: " << port.dataType;
            if (port.putType == "in") {
                if (port.dataType == SV::PortMsg::REAL || port.dataType == SV::PortMsg::INT || port.dataType == SV::PortMsg::ANYTYPE) {
                    SV::DoubleCircularBuffer *a = new SV::DoubleCircularBuffer;
                    AddInputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::CIRCULAR_BUFFER_DOUBLE);
                    qDebug() << "MATLAB_Script_Block Initialize add input double port Name: " << QString::fromStdString(GetInputPortName(i));
                } else if (port.dataType == SV::PortMsg::COMPLEX) {
                    SV::DComplexCircularBuffer *a = new SV::DComplexCircularBuffer;
                    AddInputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::CIRCULAR_BUFFER_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add input complex port Name: " << QString::fromStdString(GetInputPortName(i));
                } else if (port.dataType == SV::PortMsg::INT_MATRIX || port.dataType == SV::PortMsg::REAL_MATRIX) {
                    SV::DoubleMatrixCircularBuffer *a = new SV::DoubleMatrixCircularBuffer;
                    AddInputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::MATRIX_DOUBLE);
                    qDebug() << "MATLAB_Script_Block Initialize add input MATRIX port Name: " << QString::fromStdString(GetInputPortName(i));
                } else if (port.dataType == SV::PortMsg::COMPLEX_MATRIX) {
                    SV::DComplexMatrixCircularBuffer *a = new SV::DComplexMatrixCircularBuffer;
                    AddInputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::MATRIX_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add input MATRIX_DCOMPLEX port Name: " << QString::fromStdString(GetInputPortName(i));
                }
                inputs.append(port.name);
                i++;
            } else if (port.putType == "out") {
                if (port.dataType == SV::PortMsg::REAL || port.dataType == SV::PortMsg::INT || port.dataType == SV::PortMsg::ANYTYPE) {
                    SV::DoubleCircularBuffer *a = new SV::DoubleCircularBuffer;
                    AddOutputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::CIRCULAR_BUFFER_DOUBLE);
                    qDebug() << "MATLAB_Script_Block Initialize add output double port Name: " << QString::fromStdString(GetOutputPortName(j));
                } else if (port.dataType == SV::PortMsg::COMPLEX) {
                    SV::DComplexCircularBuffer *a = new SV::DComplexCircularBuffer;
                    AddOutputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::CIRCULAR_BUFFER_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add output complex port Name: " << QString::fromStdString(GetOutputPortName(j));
                } else if (port.dataType == SV::PortMsg::INT_MATRIX || port.dataType == SV::PortMsg::REAL_MATRIX) {
                    SV::DoubleMatrixCircularBuffer *a = new SV::DoubleMatrixCircularBuffer;
                    AddOutputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::MATRIX_DOUBLE);
                    qDebug() << "MATLAB_Script_Block Initialize add output MATRIX port Name: " << QString::fromStdString(GetOutputPortName(j));
                } else if (port.dataType == SV::PortMsg::COMPLEX_MATRIX) {
                    // BUG FIX: 原来错误地调用了 AddInputPort + GetInputPortName(i)
                    SV::DComplexMatrixCircularBuffer *a = new SV::DComplexMatrixCircularBuffer;
                    AddOutputPort(port.name.toStdString(), *a, port.portRate, DataTypes::Type::MATRIX_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add output MATRIX_DCOMPLEX port Name: " << QString::fromStdString(GetOutputPortName(j));
                }
                outputs.append(port.name);
                j++;
            }
        }

        // 将非 Equations 参数注入 Octave 工作区
        std::map<std::string, SV::Parameter> allparameters = getAllParameter();
        for (auto e : allparameters) {
            std::string Name = e.second.Name;

            if (Name != "Equations" && Name != "isUserDefined") {
                inputs.append(Name.c_str());

                QString str = e.second.Value.c_str();
                QString trimmed = str.trimmed();

                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    // === 数组 / 矩阵 ===
                    QString inner = trimmed.mid(1, trimmed.length() - 2).trimmed();
                    if (inner.isEmpty()) {
                        // 空数组 []
                        m_interp->get_evaluator().top_level_assign(Name, octave_value(::Matrix(0, 0)));
                    } else if (inner.contains(";")) {
                        // 2D 矩阵
                        assignMatrixParam(Name, inner);
                    } else {
                        // 1D 数组
                        assignArrayParam(Name, inner);
                    }
                } else if (trimmed.contains("(") || trimmed.contains(",")) {
                    // === 复数标量 (real,imag) 或 real,imag ===
                    assignComplexScalarParam(Name, trimmed);
                } else {
                    // === double 标量 ===
                    double value = std::stod(e.second.Value);
                    m_interp->get_evaluator().top_level_assign(Name, octave_value(value));
                }
            }
        }

        // 生成 MATLAB/Octave function 文件
        QString mStr = "function ";
        callStr = "";
        if (outputs.size() == 1) {
            callStr.append(outputs[0]);
        } else if (outputs.size() > 1) {
            callStr.append("[");
            callStr.append(outputs.join(","));
            callStr.append("]");
        }
        callStr.append(QString("=%1_runfc").arg(getInstanceName().c_str()));
        callStr.append("(");
        callStr.append(inputs.join(","));
        callStr.append(")");
        mStr.append(callStr);
        mStr.append("\n");
        // 确保 Equations 每行以分号结尾，防止 Octave 回显变量值
        {
            QStringList eqLines = QString::fromStdString(Equations).split("\n");
            for (int li = 0; li < eqLines.size(); ++li) {
                QString trimmed = eqLines[li].trimmed();
                if (trimmed.isEmpty() || trimmed.startsWith('%')) {
                    continue;
                }
                if (!trimmed.endsWith(';')) {
                    eqLines[li] = eqLines[li].trimmed() + ";";
                }
            }
            mStr.append(eqLines.join("\n").toUtf8().constData());
        }
        mStr.append("\n");
        mStr.append("end\n");

        QString filePath = QString("%1/%2_runfc.m").arg(folderPath).arg(getInstanceName().c_str());
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "文件打开失败：" << file.errorString();
            return false;
        }
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << mStr;
        file.close();
    }

    m_addCx = std::make_unique<MATLAB_Script>();
    SetBlockType(SV::Block::BlockType::PROCESSOR);

    qDebug() << "......................................................................................." << QString::fromStdString(Equations);
    SetDefaultParameters();

    SetParameters();
    qDebug() << "MATLAB_Script_Block Initialize end";
    return true;
}

void MATLAB_Script_Block::SetParameters()
{
}

void MATLAB_Script_Block::SetDefaultParameters()
{
}
