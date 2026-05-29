#include "Conjugate_M_Block.h"

Conjugate_M_Block::Conjugate_M_Block(const std::string& name)
    : Block(name)
{
}

bool Conjugate_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Conjugate_M_Block::Run()
{
    return DataStreamRun();
}

bool Conjugate_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Conjugate_M = std::make_unique<Conjugate_M>();

    // 注册端口（Matrix 数据）
    AddInputPort("input", m_Conjugate_M->input, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_Conjugate_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

bool Conjugate_M_Block::DataStreamRun()
{
    // 读取输入复数矩阵
    auto inputData = ReadInputData<SystemVueModelBuilder::Matrix<std::complex<double>>>(GetInputPortName(0));
    if (inputData.empty()) return false;

    const SystemVueModelBuilder::Matrix<std::complex<double>>& inMx = inputData[0];

    // 创建输出矩阵，保持相同维度
    SystemVueModelBuilder::Matrix<std::complex<double>> outMx;
    outMx.Resize(static_cast<int>(inMx.NumRows()), static_cast<int>(inMx.NumColumns()));

    // 逐元素计算共轭
    const size_t N = inMx.NumElements();
    for (size_t i = 0; i < N; ++i) {
        const std::complex<double>& v = inMx(i);
        outMx(i) = std::complex<double>(v.real(), -v.imag());
    }

    // 写入输出
    std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputVec;
    outputVec.push_back(outMx);
    WriteOutputData(GetOutputPortName(0), outputVec);

    return true;
}
