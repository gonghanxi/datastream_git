// test_octave_run.cpp
#include "DataInterface.h"
#include "ShareSerialization.h"
#include "OctaveRun.h"
#include <iostream>
#include <vector>
#include <complex>

#include "OctaveClient.h"

// 测试函数声明
bool test_basic_types();
bool test_real_matrix();
bool test_complex_matrix();
bool test_mixed_operations();

int main22222() {
    std::cout << "===== Octave 执行测试 =====" << std::endl;
    
    bool all_passed = true;
    
    // 测试1: 基本数据类型
    std::cout << "\n1. 测试基本数据类型..." << std::endl;
    if (test_basic_types()) {
        std::cout << "   ✓ 基本数据类型测试通过" << std::endl;
    } else {
        std::cout << "   ✗ 基本数据类型测试失败" << std::endl;
        all_passed = false;
    }
    
    // 测试2: 实数矩阵
    std::cout << "\n2. 测试实数矩阵..." << std::endl;
    if (test_real_matrix()) {
        std::cout << "   ✓ 实数矩阵测试通过" << std::endl;
    } else {
        std::cout << "   ✗ 实数矩阵测试失败" << std::endl;
        all_passed = false;
    }
    
    // 测试3: 复数矩阵
    std::cout << "\n3. 测试复数矩阵..." << std::endl;
    if (test_complex_matrix()) {
        std::cout << "   ✓ 复数矩阵测试通过" << std::endl;
    } else {
        std::cout << "   ✗ 复数矩阵测试失败" << std::endl;
        all_passed = false;
    }
    
    // 测试4: 混合运算
    std::cout << "\n4. 测试混合运算..." << std::endl;
    if (test_mixed_operations()) {
        std::cout << "   ✓ 混合运算测试通过" << std::endl;
    } else {
        std::cout << "   ✗ 混合运算测试失败" << std::endl;
        all_passed = false;
    }
    
    std::cout << "\n===== 测试结果 =====" << std::endl;
    if (all_passed) {
        std::cout << "所有测试通过!" << std::endl;
    } else {
        std::cout << "部分测试失败!" << std::endl;
    }
    
    return all_passed ? 0 : 1;
}

// 测试1: 基本数据类型
bool test_basic_types() {
    try {
        // 创建 Octave 运行器
//        OctaveRun octaveRunner;
        OctaveClient client(88);
        // 创建参数集
        ParamInfo &paramSet  = client.getShmInfo().pSet;
        
        // 添加输入参数
        HeadData intInHead;
        intInHead.setParamName("a");
        intInHead.dataType = DataType::INT;
        intInHead.putType = Put_In;
        intInHead.operate = CmdReadParamData;
        intInHead.bytes = sizeof(int);
        paramSet.addParamHead(intInHead);
        
        HeadData realInHead;
        realInHead.setParamName("b");
        realInHead.dataType = DataType::REAL;
        realInHead.putType = Put_In;
        realInHead.operate = CmdReadParamData;
        realInHead.bytes = sizeof(double);

        paramSet.addParamHead(realInHead);
        
        HeadData complexInHead;
        complexInHead.setParamName("c");
        complexInHead.dataType = DataType::COMPLEX;
        complexInHead.putType = Put_In;
        complexInHead.operate = CmdReadParamData;
        complexInHead.bytes = sizeof(std::complex<double>);

        paramSet.addParamHead(complexInHead);
        
        // 添加输出参数
        HeadData intOutHead;
        intOutHead.setParamName("a_out");
        intOutHead.dataType = DataType::INT;
        intOutHead.putType = Put_Out;
        intOutHead.operate = CmdReadParamData;
        paramSet.addParamHead(intOutHead);
        
        HeadData realOutHead;
        realOutHead.setParamName("b_out");
        realOutHead.dataType = DataType::REAL;
        realOutHead.putType = Put_Out;
        realOutHead.operate = CmdReadParamData;
        paramSet.addParamHead(realOutHead);
        
        HeadData complexOutHead;
        complexOutHead.setParamName("c_out");
        complexOutHead.dataType = DataType::COMPLEX;
        complexOutHead.putType = Put_Out;
        complexOutHead.operate = CmdReadParamData;
        paramSet.addParamHead(complexOutHead);
        std::cout<<"xxxxxxxxxxxxxxxxxxx"<<std::endl;
        // 设置输入数据
//        std::vector<char>& a_data = paramSet.getData("a");
        int a_value = 42;
        ShareSerialization::dataToParam(&a_value, paramSet.getParam("a"));
        
//        std::vector<char>& b_data = paramSet.getData("b");
        double b_value = 3.14159;
        ShareSerialization::dataToParam(&b_value, paramSet.getParam("b"));
        
//        std::vector<char>& c_data = paramSet.getData("c");
        std::complex<double> c_value(1.0, 2.0); // 1 + 2i
        ShareSerialization::dataToParam(&c_value, paramSet.getParam("c"));
        std::cout<<"xxxxxxxxxxxxxxxxxxx1"<<std::endl;

        // Octave 代码
        std::string code = R"(
a_out = a + 10;
b_out = b * 2.0;
c_out = c + 5.0;
)";

//        std::vector<char> headBuffer ;
//        headBuffer.resize(sizeof(HeadData));
//        bool isWrite = false;
//        for (auto it = paramSet.paramSet.begin(); it != paramSet.paramSet.end(); ++it) {
//            const Param& p = it.value();

//            std::cout <<p.head.getParamName().toStdString()<<std::endl;
//            ShareSerialization::headToBuffer(headBuffer, p.head);
//            shmInfo.write(headBuffer);
//            shmInfo.write(p.dataBuffer);
//        }
        
        // 执行代码
//        bool success = client.sendInfo(paramSet, code);

        client.sendInfo(paramSet, code);
        client.removeShmServer();
        client.waitRemoveShmServer();

        std::cout<<"xxxxxxxxxxxxxxxxxxx2"<<std::endl;

//        if (!success) {
//            std::cerr << "   Octave 代码执行失败" << std::endl;
//            return false;
//        }

        // 验证输出
        std::vector<char>& a_out_data = paramSet.getData("a_out");
        int a_out_value;
        ShareSerialization::paramToData({intOutHead, a_out_data}, &a_out_value);
        if (a_out_value != 52) {  // 42 + 10
            std::cerr << "   整数运算错误: 期望 52, 实际 " << a_out_value << std::endl;
            return false;
        }
        
        std::vector<char>& b_out_data = paramSet.getData("b_out");
        double b_out_value;
        ShareSerialization::paramToData({realOutHead, b_out_data}, &b_out_value);
        if (std::abs(b_out_value - 6.28318) > 1e-5) {  // 3.14159 * 2
            std::cerr << "   实数运算错误: 期望 ~6.28318, 实际 " << b_out_value << std::endl;
            return false;
        }
        
        std::vector<char>& c_out_data = paramSet.getData("c_out");
        std::complex<double> c_out_value;
        ShareSerialization::paramToData({complexOutHead, c_out_data}, &c_out_value);
        std::complex<double> expected(6.0, 2.0);  // (1+2i) + 5
        if (std::abs(c_out_value.real() - expected.real()) > 1e-5 ||
            std::abs(c_out_value.imag() - expected.imag()) > 1e-5) {
            std::cerr << "   复数运算错误: 期望 (6,2), 实际 ("
                      << c_out_value.real() << "," << c_out_value.imag() << ")" << std::endl;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "   异常: " << e.what() << std::endl;
        return false;
    }
}

// 测试2: 实数矩阵运算
bool test_real_matrix() {
    try {
//        OctaveRun octaveRunner;
//        ParamInfo paramSet;
        OctaveClient client(88);
        // 创建参数集
        ParamInfo &paramSet  = client.getShmInfo().pSet;
        
        // 创建 2x2 实数矩阵输入
        HeadData matInHead;
        matInHead.setParamName("M");
        matInHead.dataType = DataType::REAL_MATRIX;
        matInHead.putType = Put_In;
        matInHead.row = 2;
        matInHead.col = 2;
        matInHead.operate = CmdReadParamData;
        matInHead.bytes = 2*2*sizeof(double);
        paramSet.addParamHead(matInHead);
        
        // 创建矩阵输出
        HeadData matOutHead;
        matOutHead.setParamName("M_out");
        matOutHead.dataType = DataType::REAL_MATRIX;
        matOutHead.putType = Put_Out;
        matOutHead.operate = CmdReadParamData;
//        matOutHead.bytes = 2*2*sizeof(double);
        paramSet.addParamHead(matOutHead);
        
        // 准备输入数据
//        std::vector<char>& m_data = paramSet.getData("M");
        double matrix_input[4] = {1.0, 2.0, 3.0, 4.0};  // [1, 2; 3, 4]
        ShareSerialization::dataToParam(matrix_input, paramSet.getParam("M"));
        
        // Octave 代码: 计算矩阵的平方
        std::string code = R"(
M_out = M * M;
)";
        client.sendInfo(paramSet, code);
        
        // 执行
//        bool success = octaveRunner.runCode(paramSet, code);
//        if (!success) {
//            std::cerr << "   矩阵运算代码执行失败" << std::endl;
//            return false;
//        }
        
        // 验证结果
        std::vector<char>& m_out_data = paramSet.getData("M_out");
        double matrix_output[4];
        ShareSerialization::paramToData({matOutHead, m_out_data}, matrix_output);
        
        // 期望结果: [1,2;3,4] * [1,2;3,4] = [7,10;15,22]
        double expected[4] = {7.0, 10.0, 15.0, 22.0};
        
        for (int i = 0; i < 4; i++) {
            if (std::abs(matrix_output[i] - expected[i]) > 1e-5) {
                std::cerr << "   矩阵乘法错误: 元素[" << i << "] 期望 " 
                          << expected[i] << ", 实际 " << matrix_output[i] << std::endl;
                return false;
            }
        }
        
        std::cout << "   矩阵运算结果: [" 
                  << matrix_output[0] << ", " << matrix_output[1] << "; "
                  << matrix_output[2] << ", " << matrix_output[3] << "]" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "   异常: " << e.what() << std::endl;
        return false;
    }
}

// 测试3: 复数矩阵运算
bool test_complex_matrix() {
    try {
//
        OctaveClient client(88);

        // 创建参数集
        ParamInfo &paramSet  = client.getShmInfo().pSet;
//        ParamInfo paramSet;
        // 创建 2x2 复数矩阵输入
        HeadData cmatInHead;
        cmatInHead.setParamName("C");
        cmatInHead.dataType = DataType::COMPLEX_MATRIX;
        cmatInHead.putType = Put_In;
        cmatInHead.row = 2;
        cmatInHead.col = 2;
        cmatInHead.operate = CmdReadParamData;
        cmatInHead.bytes = 2*2*sizeof(std::complex<double>);
        paramSet.addParamHead(cmatInHead);
        
        // 创建复数矩阵输出
        HeadData cmatOutHead;
        cmatOutHead.setParamName("C_out");
        cmatOutHead.dataType = DataType::COMPLEX_MATRIX;
        cmatOutHead.putType = Put_Out;
        cmatOutHead.operate = CmdReadParamData;
        paramSet.addParamHead(cmatOutHead);
        
        // 准备输入数据
//        std::vector<char>& c_data = paramSet.getData("C");
        std::vector<std::complex<double>> cMatrix;
        cMatrix.resize(4);
//        std::complex<double> c_matrix[4] = {
        cMatrix[0]   = std::complex<double>(1.0, 0.5); // 1 + 0.5i
           cMatrix[1]   =  std::complex<double>(2.0, 1.0);  // 2 + 1i
          cMatrix[2]   =   std::complex<double>(3.0, 1.5);  // 3 + 1.5i
          cMatrix[3]   =   std::complex<double>(4.0, 2.0) ;  // 4 + 2i
//        };
//          std::cout<<sizeof(std::complex<double>)<<std::endl;
        ShareSerialization::dataToParam(cMatrix.data(), paramSet.getParam("C"));
        
        // Octave 代码: 计算共轭转置
        std::string code = R"(
C_out = C' + eye(2);
)";
//        return true;;
        // 执行

                client.sendInfo(paramSet, code);
//         OctaveRun octaveRunner;
//        bool success = octaveRunner.runCode(paramSet, code);
//        if (!success) {
//            std::cerr << "   复数矩阵运算代码执行失败" << std::endl;
//            return false;
//        }

//        return false;
        
        // 验证结果
////        std::vector<char>& c_out_data = paramSet.getData("C_out");
        std::complex<double> c_out_matrix[4];
        ShareSerialization::paramToData(paramSet.getParam("C_out"), c_out_matrix);
        
        // 输出结果用于验证
        std::cout << "   复数矩阵运算结果:" << std::endl;
        for (int i = 0; i < 2; i++) {
            std::cout << "   ";
            for (int j = 0; j < 2; j++) {
                std::complex<double> val = c_out_matrix[i + j * 2];
                std::cout << "(" << val.real() << (val.imag() >= 0 ? "+" : "")
                          << val.imag() << "i) ";
            }
            std::cout << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "   异常: " << e.what() << std::endl;
        return false;
    }
}

// 测试4: 混合运算
bool test_mixed_operations() {
    try {
        OctaveRun octaveRunner;
        OctaveClient client(88);
        // 创建参数集
        ParamInfo &paramSet  = client.getShmInfo().pSet;
        
        // 添加各种输入参数
        HeadData scalarHead;
        scalarHead.setParamName("x");
        scalarHead.dataType = DataType::REAL;
        scalarHead.putType = Put_In;
        paramSet.addParamHead(scalarHead);
        
        HeadData matHead;
        matHead.setParamName("A");
        matHead.dataType = DataType::REAL_MATRIX;
        matHead.putType = Put_In;
        matHead.row = 2;
        matHead.col = 2;
        paramSet.addParamHead(matHead);
        
        HeadData complexHead;
        complexHead.setParamName("z");
        complexHead.dataType = DataType::COMPLEX;
        complexHead.putType = Put_In;
        paramSet.addParamHead(complexHead);
        
        // 输出参数
        HeadData resultHead1;
        resultHead1.setParamName("result1");
        resultHead1.dataType = DataType::REAL_MATRIX;
        resultHead1.putType = Put_Out;
        paramSet.addParamHead(resultHead1);
        
        HeadData resultHead2;
        resultHead2.setParamName("result2");
        resultHead2.dataType = DataType::COMPLEX;
        resultHead2.putType = Put_Out;
        paramSet.addParamHead(resultHead2);
        
        // 设置输入数据
        double x_val = 2.5;
        std::vector<char>& x_data = paramSet.getData("x");
        ShareSerialization::dataToParam(&x_val, paramSet.getParam("x"));
        
        double A_mat[4] = {1.0, 2.0, 3.0, 4.0};
        std::vector<char>& A_data = paramSet.getData("A");
        ShareSerialization::dataToParam(A_mat, paramSet.getParam("A"));
        
        std::complex<double> z_val(1.0, 1.0);
        std::vector<char>& z_data = paramSet.getData("z");
        ShareSerialization::dataToParam(&z_val, paramSet.getParam("z"));
        
        // 复杂的 Octave 代码
        std::string code = R"(
% 混合运算测试
result1 = x * A + eye(2);  % 标量乘矩阵加单位矩阵
result2 = z^2 + det(A);    % 复数平方加矩阵行列式
disp('混合运算结果:');
result1
result2
)";
        
        std::cout << "   执行混合运算代码..." << std::endl;
        bool success = octaveRunner.runCode(paramSet, code);


        
        if (!success) {
            std::cerr << "   混合运算代码执行失败" << std::endl;
            return false;
        }
        
        // 获取结果
        std::vector<char>& result1_data = paramSet.getData("result1");
        double result1_mat[4];
        ShareSerialization::paramToData({resultHead1, result1_data}, result1_mat);
        
        std::vector<char>& result2_data = paramSet.getData("result2");
        std::complex<double> result2_val;
        ShareSerialization::paramToData({resultHead2, result2_data}, &result2_val);
        
        // 显示结果
        std::cout << "   混合运算结果1 (result1 矩阵):" << std::endl;
        std::cout << "   [" << result1_mat[0] << ", " << result1_mat[1] << ";" 
                  << result1_mat[2] << ", " << result1_mat[3] << "]" << std::endl;
        
        std::cout << "   混合运算结果2 (result2 复数): " 
                  << "(" << result2_val.real() << (result2_val.imag() >= 0 ? "+" : "") 
                  << result2_val.imag() << "i)" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "   异常: " << e.what() << std::endl;
        return false;
    }
}
