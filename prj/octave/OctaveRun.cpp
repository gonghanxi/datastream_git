#include "OctaveRun.h"
#include <stdio.h>

#include "ShareSerialization.h"

#include <QList>
//#include "mex.h"
OctaveRun::OctaveRun()
{

    interpreter.initialize();
//    int status = 0;


//    // 默认情况下，C++控制台程序没有图形界面
//    // 当Octave的plot函数被调用时，它会创建图形窗口
//    // 但控制台程序没有消息循环来处理窗口事件
//    // 1. 执行一些 MATLAB 代码
//    std::string code = "figure('visible', 'on'); x = 0:0.1:2*pi; y = sin(x); plot(x, y); title('正弦函数'); xlabel('X轴');ylabel('Y轴'); grid on; drawnow; pause;";
//    //    std::string code = "x = 0:0.1:2*pi; y = sin(x);;";
//    int status11 = 0;
//    interpreter.eval_string(code, true, status11, 0);

//    octave_value x_val1 = interpreter.find("x");
//    std::cout << "x = " << x_val1.is_real_matrix() << std::endl;
//    std::cout << "x = " << x_val1.is_range() << std::endl;
//    std::cout << "x = " << x_val1.is_sorted_rows() << std::endl;
//    NDArray doubleArray = x_val1.array_value();
//    int64_t size = doubleArray.numel();

//    for (int i = 0; i < size; i++) {
//        std::cout << doubleArray(i) << " ";
//    }

    //    return 0;
}

bool OctaveRun::runCode(ParamInfo &pSet, std::string &code)
{
//    interpreter.clear_all();
    QList<Param*> inParamSet;
    QList<Param*> outParamSet;

    for (auto it = pSet.paramSet.begin(); it != pSet.paramSet.end(); ++it) {
        Param& p = it.value();
        if ( p.head.putType==Put_In)
            inParamSet.append(&p);
        if ( p.head.putType==Put_Out)
            outParamSet.append(&p);
    }

    for(Param*p: inParamSet)
    {
        if (p==nullptr)
            return false;;
        std::cout<<p->head.getParamName().toStdString()<<std::endl;
        switch (p->head.dataType) {
        case DataType::INT:
        {
            int vaule;
            ShareSerialization::paramToData(*p, &vaule);
            interpreter.assign(p->head.paramName, vaule);
            break;
        }
        case DataType::REAL:
        {
            double vauleDouble;
            ShareSerialization::paramToData(*p, &vauleDouble);
            interpreter.assign(p->head.paramName, vauleDouble);
            break;
        }

        case DataType::COMPLEX:
        {
            std::complex<double> vauleComplex;
            ShareSerialization::paramToData(*p, &vauleComplex);
            interpreter.assign(p->head.paramName, vauleComplex);
            break;
        }
        case DataType::COMPLEX_MATRIX:
        {

            std::cout<<p->head.row<<"__"<<p->head.row<<std::endl;
            ComplexMatrix m(p->head.row, p->head.col);
            std::complex<double>*data = m.fortran_vec();
            ShareSerialization::paramToData(*p, data);
            interpreter.assign(p->head.paramName, m);
            break;
        }
        case DataType::REAL_MATRIX:
        {

            Matrix m(p->head.row,p->head.col);
            double* data = m.fortran_vec();
            ShareSerialization::paramToData(*p, data);
            interpreter.assign(p->head.paramName, m);
             break;
        }
        case DataType::INT_MATRIX:

            break;
        }
    }
//    return true;
    int status = 0;
    interpreter.eval_string(code, true, status, 0);
    if (status !=0)
        return false;
    for(Param*p: outParamSet)
    {
        octave_value valueOctave = interpreter.find(p->head.paramName);
        switch (p->head.dataType) {
        case DataType::INT:
        {
            int value = valueOctave.xint_value("int");
            p->head.row = 1;
            p->head.col = 1;
            p->head.bytes = sizeof(int);
            ShareSerialization::dataToParam(&value, *p);
            std::cout<<"int result:"<<value<<std::endl;
            break;
        }
        case DataType::REAL:
        {
            double vauleDouble = valueOctave.xdouble_value("real");;
            p->head.row = 1;
            p->head.col = 1;
            p->head.bytes = sizeof(double);
            ShareSerialization::dataToParam(&vauleDouble, *p);
            std::cout<<"double result:"<<vauleDouble<<std::endl;
            break;
        }

        case DataType::COMPLEX:
        {
            std::complex<double> vauleComplex = valueOctave.xcomplex_value("");
            p->head.row = 1;
            p->head.col = 1;
            p->head.bytes = sizeof(std::complex<double>);
            ShareSerialization::dataToParam(&vauleComplex, *p);
            std::cout<<"complex result:"<<vauleComplex<<std::endl;
            break;
        }
        case DataType::COMPLEX_MATRIX:
        {
//            const ComplexMatrix &m = valueOctave.xcomplex_matrix_value("sdd");
            const std::complex<double> * mptr = static_cast<const std::complex<double>*>(valueOctave.mex_get_data());
            int rows =   valueOctave.rows();
            int columns = valueOctave.columns();
            if (mptr)
            {
                p->head.row = rows;
                p->head.col = columns;
                p->head.bytes = p->head.row * p->head.col * sizeof(std::complex<double>);

                ShareSerialization::dataToParam(mptr, *p);
                std::vector<std::complex<double> > mData;
                mData.resize(rows*columns);
                ShareSerialization::paramToData(*p,mData.data());
                printMatrix(mData.data(), rows, columns);

            }else
            {
                throw std::runtime_error("读取复数矩阵错误");
            }
            break;

        }
        case DataType::REAL_MATRIX:
        {
            const double * mptr = static_cast<const double*>(valueOctave.mex_get_data());
            int rows =   valueOctave.rows();
            int columns = valueOctave.columns();
            if (mptr)
            {
                p->head.row = rows;
                p->head.col = columns;
                p->head.bytes = p->head.row * p->head.col * sizeof(double);

                ShareSerialization::dataToParam(mptr, *p);

                std::vector<double>  mData;
                mData.resize(rows*columns);
                ShareSerialization::paramToData(*p,mData.data());
                printMatrix(mData.data(), rows, columns);
            }else
            {
                throw std::runtime_error("读取复数矩阵错误");
            }
             break;
        }
        case DataType::INT_MATRIX:

            break;
        }
    }
    return true;
}

// 保存当前工作空间到文件
bool OctaveRun::saveWorkspace(const std::string& filename) {
    int status = 0;
    try {
        std::string cmd = "save('" + filename + ".mat');";
        interpreter.eval_string(cmd, true, status, 0);
        return true;
    } catch (const octave::execution_exception& e) {
        std::cerr << "保存工作空间失败: " << e.what() << std::endl;
        return false;
    }
}

// 从文件加载工作空间
bool OctaveRun::loadWorkspace(const std::string& filename) {
    int status = 0;
    try {
        // 先清除当前工作空间
        std::string cmdClear = "clear all;";
        interpreter.eval_string(cmdClear, true, status, 0);

        std::string cmd = "load('" + filename + ".mat');";
        interpreter.eval_string(cmd, true, status, 0);
        return true;
    } catch (const octave::execution_exception& e) {
        std::cerr << "加载工作空间失败: " << e.what() << std::endl;
        return false;
    }
}

// 切换到另一个工作空间（保存当前，加载目标）
bool OctaveRun::switchWorkspace(const std::string& currentName,
                    const std::string& targetName) {
    if (!saveWorkspace(currentName)) {
        return false;
    }
    return loadWorkspace(targetName);
}




