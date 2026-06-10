#ifndef OCTAVERUN_H
#define OCTAVERUN_H
#include <octave/parse.h>

#include <octave/interpreter.h>
// //#include <octave/oct-obj.h>
#include <octave/ov.h>

#include <octave/symtab.h>
#include <iostream>
//#include <processenv.h>
#include "DataInterface.h"

#include <iomanip>
class OctaveRun
{
public:
    OctaveRun();
    bool runCode(ParamInfo &pSet, std::string &code);

//    template <typename T>
//    void print(T*dataPtr, int rowNum, int colNum)
//    {

//    }

    // 假设已知矩阵的行数 rows 和列数 cols
     template <typename T>
    void printMatrix(const T* dataptr, int rows, int cols) {
        std::cout << "矩阵 (" << rows << "x" << cols << "):" << std::endl;
        std::cout << std::fixed << std::setprecision(6);  // 设置输出精度

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // 注意：Octave/Matlab 使用列优先存储
                std::cout << std::setw(12) << dataptr[i + j * rows] << " ";
            }
            std::cout << std::endl;
        }
    }

    template <typename T>
   void printMatrixComplex(const T* dataptr, int rows, int cols) {
       std::cout << "矩阵 (" << rows << "x" << cols << "):" << std::endl;
       std::cout << std::fixed << std::setprecision(6);  // 设置输出精度

       for (int i = 0; i < rows; i++) {
           for (int j = 0; j < cols; j++) {
               // 注意：Octave/Matlab 使用列优先存储
               std::cout << std::setw(12) << dataptr[i + j * rows] << " ";
           }
           std::cout << std::endl;
       }
   }

private:
   octave::interpreter interpreter;
   bool switchWorkspace(const std::string &currentName, const std::string &targetName);
   bool loadWorkspace(const std::string &filename);
   bool saveWorkspace(const std::string &filename);
};

#endif // OCTAVERUN_H
