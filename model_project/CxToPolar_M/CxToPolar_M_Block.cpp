#include "CxToPolar_M_Block.h"

#include <complex>
#include <vector>

CxToPolar_M_Block::CxToPolar_M_Block(const std::string& name)
    : Block(name)
{
}

bool CxToPolar_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool CxToPolar_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_CxToPolar_M = std::make_unique<CxToPolar_M>();

    AddInputPort("input", m_CxToPolar_M->input, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("magnitude", m_CxToPolar_M->magnitude, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("phase", m_CxToPolar_M->phase, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

bool CxToPolar_M_Block::Run()
{
    // 读取输入复数矩阵
    auto inputData = ReadInputData<SystemVueModelBuilder::Matrix<std::complex<double>>>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::Matrix<std::complex<double>>& inMx = inputData[0];
    const size_t NRow = inMx.NumRows();
    const size_t NCol = inMx.NumColumns();

    // 创建两个输出矩阵
    SystemVueModelBuilder::Matrix<double> magMx;
    SystemVueModelBuilder::Matrix<double> phaseMx;
    magMx.Resize(static_cast<int>(NRow), static_cast<int>(NCol));
    phaseMx.Resize(static_cast<int>(NRow), static_cast<int>(NCol));

    // 逐元素计算幅值和相位
    for (size_t row = 0; row < NRow; ++row)
    {
        for (size_t col = 0; col < NCol; ++col)
        {
            magMx(row, col) = std::abs(inMx(row, col));
            phaseMx(row, col) = std::arg(inMx(row, col));
        }
    }

    // 写入输出
    std::vector<SystemVueModelBuilder::Matrix<double>> magVec;
    magVec.push_back(magMx);
    WriteOutputData(GetOutputPortName(0), magVec);

    std::vector<SystemVueModelBuilder::Matrix<double>> phaseVec;
    phaseVec.push_back(phaseMx);
    WriteOutputData(GetOutputPortName(1), phaseVec);

    return true;
}
