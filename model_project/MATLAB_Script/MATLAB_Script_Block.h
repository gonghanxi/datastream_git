#ifndef ADDCX_BLOCK_H
#define ADDCX_BLOCK_H

#include "MATLAB_Script.h"
#include "Block.h"
#include <octave/interpreter.h>
#include <octave/ov.h>
#include <octave/Matrix.h>
//#include <octave/ComplexMatrix.h>
#include <octave/Cell.h>

class SYSTEMVUEMODELBUILDER_API MATLAB_Script_Block : public SystemVueModelBuilder::Block
{
public:
    MATLAB_Script_Block(const std::string& name);
    ~MATLAB_Script_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<MATLAB_Script> m_addCx;

    // Octave 数据转换辅助方法
    octave_value vectorToOctave(const std::vector<double>& data);
    octave_value vectorToOctave(const std::vector<std::complex<double>>& data);
    octave_value matrixToOctave(const std::vector<SystemVueModelBuilder::DoubleMatrix>& data);
    octave_value complexMatrixToOctave(const std::vector<SystemVueModelBuilder::DComplexMatrix>& data);

    // 参数解析辅助方法（数组/矩阵/复数）
    void assignArrayParam(const std::string& name, const QString& innerStr);
    void assignMatrixParam(const std::string& name, const QString& innerStr);
    void assignComplexScalarParam(const std::string& name, const QString& str);
    static std::complex<double> parseComplexElement(const QString& str);
    static bool isComplexElement(const QString& str);

    // 检查输出数据是否包含 Inf/NaN
    static bool hasInvalidValues(const std::vector<double>& data);
    static bool hasInvalidValues(const std::vector<std::complex<double>>& data);
private:
    // 共享 Octave 解释器（所有实例共用）
    static octave::interpreter* s_sharedInterp;
    static int s_instanceCount;

    octave::interpreter *m_interp;  // 指向共享解释器的指针
    QString callStr;
};

namespace SystemVueModelBuilder {
    RegAlgo(MATLAB_Script_Block);
}

#endif // ADDCX_BLOCK_H
