#include "RectToCx_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

RectToCx_M_Block::RectToCx_M_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool RectToCx_M_Block::Setup()
{
    Block::Setup();

    m_realBuffer.clear();
    m_imagBuffer.clear();
    m_outputQueue = std::queue<SystemVueModelBuilder::Matrix<std::complex<double>>>();

    return true;
}

bool RectToCx_M_Block::Run()
{
    if (IsVariableStepMode()) { return TimeDrivenRun(); }
    return DataStreamRun();
}

bool RectToCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_RectToCx_M = std::make_unique<RectToCx_M>();

    // 注册端口（Matrix 数据）
    AddInputPort("real", m_RectToCx_M->real, 1, Block::DataType::MATRIX_DOUBLE);
    AddInputPort("imag", m_RectToCx_M->imag, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_RectToCx_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool RectToCx_M_Block::DataStreamRun()
{
    // 读取 real 和 imag 端口数据
    auto realData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(0));
    auto imagData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(1));

    bool hasReal = !realData.empty();
    bool hasImag = !imagData.empty();

    // 分别获取两个输入矩阵维度，端口为空则默认 1x1
    size_t NRowReal = hasReal ? realData[0].NumRows()    : 1;
    size_t NColReal = hasReal ? realData[0].NumColumns() : 1;
    size_t NRowImag = hasImag ? imagData[0].NumRows()    : 1;
    size_t NColImag = hasImag ? imagData[0].NumColumns() : 1;

    // 输出矩阵大小取两者最大值
    const size_t NRow = NRowReal > NRowImag ? NRowReal : NRowImag;
    const size_t NCol = NColReal > NColImag ? NColReal : NColImag;

    // 创建输出矩阵
    SystemVueModelBuilder::Matrix<std::complex<double>> outputMatrix;
    outputMatrix.Resize(static_cast<int>(NRow), static_cast<int>(NCol));

    // 填充：各自只在自身维度区域内写入
    for (size_t row = 0; row < NRow; ++row) {
        for (size_t col = 0; col < NCol; ++col) {
            outputMatrix(row, col) = 0.0;
            if (hasReal && row < NRowReal && col < NColReal) {
                outputMatrix(row, col).real(realData[0](row, col));
            }
            if (hasImag && row < NRowImag && col < NColImag) {
                outputMatrix(row, col).imag(imagData[0](row, col));
            }
        }
    }

    // 写入输出
    std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputVec;
    outputVec.push_back(outputMatrix);
    WriteOutputData(GetOutputPortName(0), outputVec);

    return true;
}

// ============================================================================
// TimeDrivenRun：变步长逐点处理 — real/imag 均为可选，任一非空即触发
// ============================================================================

bool RectToCx_M_Block::TimeDrivenRun()
{
    // ① 累积 real / imag 到 vector
    {
        auto realData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(0));
        auto imagData = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(1));
        for (auto& v : realData) m_realBuffer.push_back(v);
        for (auto& v : imagData) m_imagBuffer.push_back(v);
    }

    // ② real 或 imag 任一非空 → 取首元素处理
    if (!m_realBuffer.empty() || !m_imagBuffer.empty())
    {
        bool hasReal = !m_realBuffer.empty();
        bool hasImag = !m_imagBuffer.empty();

        SystemVueModelBuilder::Matrix<double> realMat = hasReal ? m_realBuffer[0] : SystemVueModelBuilder::Matrix<double>();
        SystemVueModelBuilder::Matrix<double> imagMat = hasImag ? m_imagBuffer[0] : SystemVueModelBuilder::Matrix<double>();

        // 独立获取 real/imag 矩阵维度
        size_t NRowReal = hasReal ? realMat.NumRows()    : 1;
        size_t NColReal = hasReal ? realMat.NumColumns() : 1;
        size_t NRowImag = hasImag ? imagMat.NumRows()    : 1;
        size_t NColImag = hasImag ? imagMat.NumColumns() : 1;

        const size_t NRow = NRowReal > NRowImag ? NRowReal : NRowImag;
        const size_t NCol = NColReal > NColImag ? NColReal : NColImag;

        SystemVueModelBuilder::Matrix<std::complex<double>> outputMatrix;
        outputMatrix.Resize(static_cast<int>(NRow), static_cast<int>(NCol));

        for (size_t row = 0; row < NRow; ++row) {
            for (size_t col = 0; col < NCol; ++col) {
                outputMatrix(row, col) = 0.0;
                if (hasReal && row < NRowReal && col < NColReal) {
                    outputMatrix(row, col).real(realMat(row, col));
                }
                if (hasImag && row < NRowImag && col < NColImag) {
                    outputMatrix(row, col).imag(imagMat(row, col));
                }
            }
        }

        m_outputQueue.push(outputMatrix);
    }

    // ③ 出队写入，输出后清空输入 buffer
    if (!m_outputQueue.empty()) {
        SystemVueModelBuilder::Matrix<std::complex<double>> out = m_outputQueue.front();
        m_outputQueue.pop();
        std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputVec;
        outputVec.push_back(out);
        WriteOutputData(GetOutputPortName(0), outputVec);

        m_realBuffer.clear();
        m_imagBuffer.clear();
    } else {
        // 无输入也无待输出 → 输出 1x1 零矩阵
        SystemVueModelBuilder::Matrix<std::complex<double>> zeroOutput;
        zeroOutput.Resize(1, 1);
        zeroOutput(0, 0) = 0.0;
        std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputVec;
        outputVec.push_back(zeroOutput);
        WriteOutputData(GetOutputPortName(0), outputVec);
    }

    return true;
}
