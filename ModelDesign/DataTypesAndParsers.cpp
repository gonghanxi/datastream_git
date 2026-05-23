#include "DataTypesAndParsers.h"
#include <sstream>
#include <algorithm>
#include <cctype>

bool DataTypesAndParsers::IsBusType(DataTypes::Type type) {
    //判断是否为bus类型
    switch (type) {
    case DataType::INT_BUS:
    case DataType::DOUBLE_BUS:
    case DataType::FLOAT_BUS:
    case DataType::BOOL_BUS:
    case DataType::CHAR_BUS:
    case DataType::FCOMPLEX_BUS:
    case DataType::DCOMPLEX_BUS:
    case DataType::ENVELOPE_BUS:
    case DataType::MATRIX_INT_BUS:
    case DataType::MATRIX_DOUBLE_BUS:
    case DataType::MATRIX_FLOAT_BUS:
    case DataType::MATRIX_BOOL_BUS:
    case DataType::MATRIX_FCOMPLEX_BUS:
    case DataType::MATRIX_DCOMPLEX_BUS:
    case DataType::MATRIX_ENVELOPE_BUS:
        return true;
    default:
        return false;
    }
}

// 获取数据类型大小
size_t DataTypesAndParsers::GetDataTypeSize(DataTypes::Type type) {
    switch (type) {
    case DataType::INT:
    case DataType::TIMED_INT:
    case DataType::CIRCULAR_BUFFER_INT:
        return sizeof(int);
    case DataType::DOUBLE:
    case DataType::TIMED_DOUBLE:
    case DataType::CIRCULAR_BUFFER_DOUBLE:
        return sizeof(double);
    case DataType::FLOAT:
    case DataType::TIMED_FLOAT:
    case DataType::CIRCULAR_BUFFER_FLOAT:
        return sizeof(float);
    case DataType::BOOL:
    case DataType::TIMED_BOOL:
    case DataType::CIRCULAR_BUFFER_BOOL:
        return sizeof(bool);
    case DataType::COMPLEX_FLOAT:
    case DataType::TIMED_FCOMPLEX:
    case DataType::CIRCULAR_BUFFER_FCOMPLEX:
        return sizeof(std::complex<float>);
    case DataType::COMPLEX_DOUBLE:
    case DataType::TIMED_DCOMPLEX:
    case DataType::CIRCULAR_BUFFER_DCOMPLEX:
        return sizeof(std::complex<double>);
    case DataType::ENVELOPE_SIGNAL:
        return sizeof(SystemVueModelBuilder::EnvelopeSignal);
    case DataType::MATRIX_INT:
        return sizeof(SystemVueModelBuilder::IntMatrix);
    case DataType::MATRIX_DOUBLE:
        return sizeof(SystemVueModelBuilder::DoubleMatrix);
    case DataType::MATRIX_FLOAT:
        return sizeof(SystemVueModelBuilder::FloatMatrix);
    case DataType::MATRIX_BOOL:
        return sizeof(SystemVueModelBuilder::BoolMatrix);
    case DataType::MATRIX_FCOMPLEX:
        return sizeof(SystemVueModelBuilder::FComplexMatrix);
    case DataType::MATRIX_DCOMPLEX:
        return sizeof(SystemVueModelBuilder::DComplexMatrix);
    case DataType::MATRIX_ENVELOPE:
        return sizeof(SystemVueModelBuilder::EnvelopeMatrix);
    default:
        return 0; // 复杂类型或总线类型
    }
}

// 字符串分割函数
std::vector<std::string> DataTypesAndParsers::SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        // 去除空白字符
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

// 解析矩阵维度
std::pair<size_t, size_t> DataTypesAndParsers::ParseMatrixDimensions(const std::string& str) {
    // 假设字符串格式为 "[rows x cols]"
    size_t rows = 1, cols = 1;

    // 这里实现具体的维度解析逻辑
    // 示例实现：查找 'x' 字符分隔行和列
    size_t xPos = str.find('x');
    if (xPos != std::string::npos) {
        std::string rowStr = str.substr(0, xPos);
        std::string colStr = str.substr(xPos + 1);

        // 去除可能的括号和空格
        rowStr.erase(std::remove_if(rowStr.begin(), rowStr.end(), ::isspace), rowStr.end());
        colStr.erase(std::remove_if(colStr.begin(), colStr.end(), ::isspace), colStr.end());

        // 去除可能的 '[' 和 ']'
        if (!rowStr.empty() && (rowStr[0] == '[' || rowStr[0] == '(')) {
            rowStr = rowStr.substr(1);
        }
        if (!colStr.empty() && (colStr.back() == ']' || colStr.back() == ')')) {
            colStr = colStr.substr(0, colStr.size() - 1);
        }

        try {
            rows = std::stoul(rowStr);
            cols = std::stoul(colStr);
        } catch (const std::exception& e) {
            // 记录解析失败，但继续使用默认值
            qDebug() << "Warning: Failed to parse dimensions from '" << QString::fromStdString(str)
                      << "': " << e.what() << ". Using default 1x1.";
            // rows 和 cols 保持为 1（已经在开头初始化为 1）
        }
    }

    return {rows, cols};
}

// 各种矩阵解析函数实现
SystemVueModelBuilder::Matrix<double> DataTypesAndParsers::ParseStringToMatrixDouble(const std::string& str) {
    SystemVueModelBuilder::Matrix<double> matrix;
    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixDouble";
            return matrix; // 返回空矩阵
    }
    //去掉首尾的方括号
    std::string content = str;
    if(content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    //统计行数和列数
    size_t rows = 1;
    size_t cols = 1;

    //检查是否有分号（表示多行）
    size_t semicolon_pos = content.find(';');
    if(semicolon_pos != std::string::npos) {
        //有多行
        rows = std::count(content.begin(), content.end(), ';') + 1;

        //获取第一行以确定列数
        std::string first_line = content.substr(0, semicolon_pos);
        cols = std::count(first_line.begin(), first_line.end(), ',') + 1;
    }
    else {
        //只有一行
        cols = std::count(content.begin(), content.end(), ',') + 1;
    }

    //调整矩阵大小
    matrix.Resize(rows, cols);

    //解析数值
    std::stringstream ss(content);
    std::string token;
    size_t row = 0;
    size_t col = 0;

    while(std::getline(ss, token, ';')) {
        std::stringstream line_ss(token);
        std::string value_str;
        col = 0;
        while(std::getline(line_ss, value_str, ',')) {
            //去除空白字符
            //remove_if是一个迭代器，第三个参数为回调函数，为真则将其指向的参数移到尾部
            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            if(!value_str.empty()) {
                matrix(row,col) = std::stod(value_str);
            col++;
        }
    }
    row++;
    }
    return matrix;
}

SystemVueModelBuilder::Matrix<int> DataTypesAndParsers::ParseStringToMatrixInt(const std::string &str)
{
    SystemVueModelBuilder::Matrix<int> matrix;

    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixInt";
            return matrix; // 返回空矩阵
    }

    //去掉首尾的方括号
    std::string content = str;
    if(content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    //统计行数和列数
    size_t rows = 1;
    size_t cols = 1;

    //检查是否有分号（表示多行）
    size_t semicolon_pos = content.find(';');
    if(semicolon_pos != std::string::npos) {
        //有多行
        rows = std::count(content.begin(), content.end(), ';') + 1;

        //获取第一行以确定列数
        std::string first_line = content.substr(0, semicolon_pos);
        cols = std::count(first_line.begin(), first_line.end(), ',') + 1;
    }
    else {
        //只有一行
        cols = std::count(content.begin(), content.end(), ',') + 1;
    }

    //调整矩阵大小
    matrix.Resize(rows, cols);

    //解析数值
    std::stringstream ss(content);
    std::string token;
    size_t row = 0;
    size_t col = 0;

    while(std::getline(ss, token, ';')) {
        std::stringstream line_ss(token);
        std::string value_str;
        col = 0;
        while(std::getline(line_ss, value_str, ',')) {
            //去除空白字符
            //remove_if是一个迭代器，第三个参数为回调函数，为真则将其指向的参数移到尾部
            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            if(!value_str.empty()) {
                matrix(row,col) = std::stoi(value_str);
                col++;
            }
        }
        row++;
    }
    return matrix;
}

SystemVueModelBuilder::Matrix<bool> DataTypesAndParsers::ParseStringToMatrixBool(const std::string &str)
{
    SystemVueModelBuilder::Matrix<bool> matrix;

    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixBool";
            return matrix; // 返回空矩阵
    }

    //去掉首尾的方括号
    std::string content = str;
    if(content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    //统计行数和列数
    size_t rows = 1;
    size_t cols = 1;

    //检查是否有分号（表示多行）
    size_t semicolon_pos = content.find(';');
    if(semicolon_pos != std::string::npos) {
        //有多行
        rows = std::count(content.begin(), content.end(), ';') + 1;

        //获取第一行以确定列数
        std::string first_line = content.substr(0, semicolon_pos);
        cols = std::count(first_line.begin(), first_line.end(), ',') + 1;
    }
    else {
        //只有一行
        cols = std::count(content.begin(), content.end(), ',') + 1;
    }

    //调整矩阵大小
    matrix.Resize(rows, cols);

    //解析数值
    std::stringstream ss(content);
    std::string token;
    size_t row = 0;
    size_t col = 0;

    while(std::getline(ss, token, ';')) {
        std::stringstream line_ss(token);
        std::string value_str;
        col = 0;
        while(std::getline(line_ss, value_str, ',')) {
            //去除空白字符
            //remove_if是一个迭代器，第三个参数为回调函数，为真则将其指向的参数移到尾部
            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            if(!value_str.empty()) {
                if(value_str == "true" || value_str == "TRUE" || value_str == "True" || value_str == "1") {
                    matrix(row, col) = true;
                }
                else {
                    matrix(row, col) = false;
                }
                col++;
            }
        }
        row++;
    }
    return matrix;
}

SystemVueModelBuilder::Matrix<char> DataTypesAndParsers::ParseStringToMatrixChar(const std::string &str)
{
    SystemVueModelBuilder::Matrix<char> matrix;

    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixChar";
            return matrix; // 返回空矩阵
    }

    //去掉首尾的方括号
    std::string content = str;
    if(content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    //统计行数和列数
    size_t rows = 1;
    size_t cols = 1;

    //检查是否有分号（表示多行）
    size_t semicolon_pos = content.find(';');
    if(semicolon_pos != std::string::npos) {
        //有多行
        rows = std::count(content.begin(), content.end(), ';') + 1;

        //获取第一行以确定列数
        std::string first_line = content.substr(0, semicolon_pos);
        cols = std::count(first_line.begin(), first_line.end(), ',') + 1;
    }
    else {
        //只有一行
        cols = std::count(content.begin(), content.end(), ',') + 1;
    }

    //调整矩阵大小
    matrix.Resize(rows, cols);

    //解析数值
    std::stringstream ss(content);
    std::string token;
    size_t row = 0;
    size_t col = 0;

    while(std::getline(ss, token, ';')) {
        std::stringstream line_ss(token);
        std::string value_str;
        col = 0;
        while(std::getline(line_ss, value_str, ',')) {
            //去除空白字符
            //remove_if是一个迭代器，第三个参数为回调函数，为真则将其指向的参数移到尾部
            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            if(!value_str.empty()) {
                if(value_str.length() == 3  && value_str[0] == '\'' && value_str[2] == '\'') {
                    matrix(row, col) = value_str[1];
                }
                else {
                    matrix(row, col) = static_cast<char>(std::stoi(value_str));
                }
                col++;
            }
        }
        row++;
    }
    return matrix;
}

SystemVueModelBuilder::Matrix<float> DataTypesAndParsers::ParseStringToMatrixFloat(const std::string &str)
{
    SystemVueModelBuilder::Matrix<float> matrix;

    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixFloat";
            return matrix; // 返回空矩阵
    }

    //去掉首尾的方括号
    std::string content = str;
    if(content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    //统计行数和列数
    size_t rows = 1;
    size_t cols = 1;

    //检查是否有分号（表示多行）
    size_t semicolon_pos = content.find(';');
    if(semicolon_pos != std::string::npos) {
        //有多行
        rows = std::count(content.begin(), content.end(), ';') + 1;

        //获取第一行以确定列数
        std::string first_line = content.substr(0, semicolon_pos);
        cols = std::count(first_line.begin(), first_line.end(), ',') + 1;
    }
    else {
        //只有一行
        cols = std::count(content.begin(), content.end(), ',') + 1;
    }

    //调整矩阵大小
    matrix.Resize(rows, cols);

    //解析数值
    std::stringstream ss(content);
    std::string token;
    size_t row = 0;
    size_t col = 0;

    while(std::getline(ss, token, ';')) {
        std::stringstream line_ss(token);
        std::string value_str;
        col = 0;
        while(std::getline(line_ss, value_str, ',')) {
            //去除空白字符
            //remove_if是一个迭代器，第三个参数为回调函数，为真则将其指向的参数移到尾部
            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            if(!value_str.empty()) {
                matrix(row, col) = std::stof(value_str);
                col++;
            }
        }
        row++;
    }
    return matrix;
}

SystemVueModelBuilder::Matrix<std::complex<float>> DataTypesAndParsers::ParseStringToMatrixFComplex(const std::string &str)
{
    SystemVueModelBuilder::Matrix<std::complex<float>> matrix;

    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixFComplex";
            return matrix; // 返回空矩阵
    }


    std::string content = str;
    if (content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    if (content.empty()) {
        matrix.Resize(0, 0);
        return matrix;
    }

    size_t rows = 1;
    size_t cols = 1;

    // 对于复数，需要特殊处理逗号
    size_t semicolon_pos = content.find(';');
    if (semicolon_pos != std::string::npos) {
        rows = std::count(content.begin(), content.end(), ';') + 1;

        // 获取第一行并计算列数（考虑复数内部的逗号）
        std::string first_line = content.substr(0, semicolon_pos);
        size_t open_paren = 0;
        for (size_t i = 0; i < first_line.length(); i++) {
            if (first_line[i] == '(') open_paren++;
            else if (first_line[i] == ')') open_paren--;
            else if (first_line[i] == ',' && open_paren == 0) {
                cols++;
            }
        }
    } else {
        // 只有一行
        size_t open_paren = 0;
        for (size_t i = 0; i < content.length(); i++) {
            if (content[i] == '(') open_paren++;
            else if (content[i] == ')') open_paren--;
            else if (content[i] == ',' && open_paren == 0) {
                cols++;
            }
        }
    }

    matrix.Resize(rows, cols);

    std::stringstream ss(content);
    std::string token;
    size_t row = 0;

    while (std::getline(ss, token, ';')) {
        size_t col = 0;
        size_t pos = 0;
        size_t open_paren = 0;

        while (col < cols && pos < token.length()) {
            // 找到下一个分隔逗号（不在括号内的逗号）
            size_t next_comma = std::string::npos;
            open_paren = 0;

            for (size_t i = pos; i < token.length(); i++) {
                if (token[i] == '(') open_paren++;
                else if (token[i] == ')') open_paren--;
                else if (token[i] == ',' && open_paren == 0) {
                    next_comma = i;
                    break;
                }
            }

            std::string value_str;
            if (next_comma != std::string::npos) {
                value_str = token.substr(pos, next_comma - pos);
                pos = next_comma + 1;
            } else {
                value_str = token.substr(pos);
                pos = token.length();
            }

            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            if (!value_str.empty()) {
                // 处理括号格式: (real, imag)
                if (value_str.front() == '(' && value_str.back() == ')') {
                    std::string inner = value_str.substr(1, value_str.length() - 2);
                    size_t comma_pos = inner.find(',');

                    if (comma_pos != std::string::npos) {
                        std::string real_str = inner.substr(0, comma_pos);
                        real_str.erase(std::remove_if(real_str.begin(), real_str.end(), ::isspace), real_str.end());
                        std::string imag_str = inner.substr(comma_pos + 1);
                        imag_str.erase(std::remove_if(imag_str.begin(), imag_str.end(), ::isspace), imag_str.end());
                        float real = std::stof(real_str);
                        float imag = std::stof(imag_str);
                        matrix(row, col) = std::complex<float>(real,imag);
                        col++;
                    }
                }
                row++;
            }
        }
    }
    return matrix;
}

SystemVueModelBuilder::Matrix<std::complex<double>> DataTypesAndParsers::ParseStringToMatrixDComplex(const std::string &str)
{
    SystemVueModelBuilder::Matrix<std::complex<double> > matrix;

    if(str.empty()) {
            qDebug() << "Warning: Empty string passed to ParseStringToMatrixDComplex";
            return matrix; // 返回空矩阵
    }

    qDebug() << "ParseStringToMatrixDComplex 输入:" << QString::fromStdString(str);

    std::string content = str;
    if (content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }

    qDebug() << "处理后的内容:" << QString::fromStdString(content);

    if (content.empty()) {
        matrix.Resize(0, 0);
        return matrix;
    }

    size_t rows = 1;
    size_t cols = 1;

    // 对于复数，需要特殊处理逗号
    size_t semicolon_pos = content.find(';');
    if (semicolon_pos != std::string::npos) {
        rows = std::count(content.begin(), content.end(), ';') + 1;

        // 获取第一行并计算列数（考虑复数内部的逗号）
        std::string first_line = content.substr(0, semicolon_pos);
        size_t open_paren = 0;
        cols = 1; // 起始为1，每发现一个不在括号内的逗号就加1

        for (size_t i = 0; i < first_line.length(); i++) {
            if (first_line[i] == '(') open_paren++;
            else if (first_line[i] == ')') open_paren--;
            else if (first_line[i] == ',' && open_paren == 0) {
                cols++;
            }
        }
    } else {
        // 只有一行
        size_t open_paren = 0;
        cols = 1; // 起始为1

        for (size_t i = 0; i < content.length(); i++) {
            if (content[i] == '(') open_paren++;
            else if (content[i] == ')') open_paren--;
            else if (content[i] == ',' && open_paren == 0) {
                cols++;
            }
        }
    }

    //qDebug() << "矩阵维度: rows =" << rows << ", cols =" << cols;
    matrix.Resize(rows, cols);

    std::stringstream ss(content);
    std::string line;
    size_t row = 0;

    while (std::getline(ss, line, ';')&& row < rows) {
        //qDebug() << "deal with" << row << ":" << QString::fromStdString(line);

        size_t col = 0;
        size_t pos = 0;
        size_t open_paren = 0;

        while (col < cols && pos < line.length()) {
            // 跳过空白字符
            while (pos < line.length() && std::isspace(line[pos])) pos++;
            if (pos >= line.length()) break;
            // 找到下一个分隔逗号（不在括号内的逗号）
            size_t next_comma = std::string::npos;
            open_paren = 0;
            bool found = false;

            for (size_t i = pos; i < line.length(); i++) {
                if (line[i] == '(') open_paren++;
                else if (line[i] == ')') open_paren--;
                else if (line[i] == ',' && open_paren == 0) {
                    next_comma = i;
                    found = true;
                    break;
                }
            }

            std::string value_str;
            if (found) {
                value_str = line.substr(pos, next_comma - pos);
                pos = next_comma + 1;
            } else {
                value_str = line.substr(pos);
                pos = line.length();
            }

            value_str.erase(std::remove_if(value_str.begin(), value_str.end(), ::isspace), value_str.end());

            //qDebug() << "解析到的字符串: " << QString::fromStdString(value_str);

            if (!value_str.empty()) {
                std::complex<double> value(0.0, 0.0);

                // 情况1: 括号格式 (real, imag)
                if (value_str.front() == '(' && value_str.back() == ')') {
                    std::string inner = value_str.substr(1, value_str.length() - 2);
                    size_t comma_pos = inner.find(',');

                    if (comma_pos != std::string::npos) {
                        std::string real_str = inner.substr(0, comma_pos);
                        std::string imag_str = inner.substr(comma_pos + 1);
                        real_str.erase(std::remove_if(real_str.begin(), real_str.end(), ::isspace), real_str.end());
                        imag_str.erase(std::remove_if(imag_str.begin(), imag_str.end(), ::isspace), imag_str.end());

                        try {
                            double real = std::stod(real_str);
                            double imag = std::stod(imag_str);
                            value = std::complex<double>(real, imag);
                        } catch (...) {
                            qDebug() << "解析括号复数失败: " << QString::fromStdString(value_str);
                        }
                    }
                }
                // 情况2: 工程复数格式（带j）
                else if (value_str.find('j') != std::string::npos || value_str.find('J') != std::string::npos) {

                    // 处理复数: -30+2j, -5-3j, 2j, -3j, 5+j2等
                    std::string real_part = "0";
                    std::string imag_part = "0";

                    // 查找j的位置
                    size_t j_pos = value_str.find_first_of("jJ");

                    // 处理纯虚数: 2j, -3j
                    if (j_pos == 0 || (j_pos == 1 && (value_str[0] == '+' || value_str[0] == '-'))) {
                        // 纯虚数
                        imag_part = value_str.substr(0, j_pos + 1);
                        imag_part.pop_back(); // 去掉j
                        if (imag_part.empty() || imag_part == "+") imag_part = "1";
                        else if (imag_part == "-") imag_part = "-1";
                    }
                    else {
                        // 查找实部和虚部的分界（+或-，但要考虑复数前的正负号）
                        // 查找实部和虚部的分界（+或-，但要考虑复数前的正负号）
                        size_t op_pos = std::string::npos;
                        for (size_t i = 1; i < value_str.length(); i++) {
                            if ((value_str[i] == '+' || value_str[i] == '-') && i > 0) {
                                // 确保这不是指数符号（e/E）
                                if (value_str[i-1] != 'e' && value_str[i-1] != 'E') {
                                    op_pos = i;
                                    break;
                                }
                            }
                        }

                        if (op_pos != std::string::npos) {
                            // 实部: 从开头到运算符之前
                            real_part = value_str.substr(0, op_pos);
                            // 虚部: 从运算符到j（包含j）
                            std::string imag_with_j = value_str.substr(op_pos, j_pos - op_pos + 1);
                            // 去掉末尾的j
                            imag_part = imag_with_j.substr(0, imag_with_j.length() - 1);
                        } else {
                            // 可能是纯虚数或纯实数
                            if (j_pos == 0 || (j_pos == 1 && (value_str[0] == '+' || value_str[0] == '-'))) {
                                // 纯虚数: 2j, -3j
                                imag_part = value_str.substr(0, j_pos);
                                if (imag_part.empty() || imag_part == "+") imag_part = "1";
                                else if (imag_part == "-") imag_part = "-1";
                                real_part = "0";
                            } else {
                                // 纯实数（理论上不应该到这里，因为已经检测到j）
                                real_part = value_str.substr(0, j_pos);
                                imag_part = "0";
                            }
                        }
                    }

                    // 清理空白
                    real_part.erase(std::remove_if(real_part.begin(), real_part.end(), ::isspace), real_part.end());
                    imag_part.erase(std::remove_if(imag_part.begin(), imag_part.end(), ::isspace), imag_part.end());

                    qDebug() << "DataTypesAndParsers::ParseStringToMatrixDComplex - real_part: " << QString::fromStdString(real_part);
                    qDebug() << "DataTypesAndParsers::ParseStringToMatrixDComplex - imag_part: " << QString::fromStdString(imag_part);

                    try {
                        double real = real_part.empty() ? 0.0 : std::stod(real_part);
                        double imag = imag_part.empty() ? 0.0 : std::stod(imag_part);
                        value = std::complex<double>(real, imag);
                    } catch (...) {
                        qDebug() << "解析工程复数失败: " << QString::fromStdString(value_str);
                    }
                }
                // 情况3: 纯实数
                else {
                    try {
                        double real = std::stod(value_str);
                        value = std::complex<double>(real, 0.0);
                    } catch (...) {
                        qDebug() << "解析实数失败: " << QString::fromStdString(value_str);
                    }
                }

                matrix(row, col) = value;
                col++;
            }
        }
        row++;
    }
    // 调试输出矩阵内容
    //qDebug() << "解析完成的矩阵:";
    for (size_t r = 0; r < rows; r++) {
        QString row_str;
        for (size_t c = 0; c < cols; c++) {
            row_str += QString("(%1, %2) ").arg(matrix(r, c).real()).arg(matrix(r, c).imag());
        }
        //qDebug() << "row " << r << ":" << row_str;
    }

    return matrix;
}
