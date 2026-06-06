#include "RADAR_NonCoIntgr_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

RADAR_NonCoIntgr_M_Block::RADAR_NonCoIntgr_M_Block(const std::string& name)
    : Block(name)
    , m_Number(5)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_NonCoIntgr_M_Block::SetDefaultParameters()
{
    m_Number = 5;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_NonCoIntgr_M_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->Number = m_Number;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_NonCoIntgr_M_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_NonCoIntgr_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// 四种情况与原算法 Run() 完全一致：
//   1. nRows == Number  → 按行积分,输出 1×nCols
//   2. nCols == Number  → 按列积分,输出 nRows×1
//   3. nRows % Number == 0 → 行分块积分
//   4. nCols % Number == 0 → 列分块积分
// ============================================================================

bool RADAR_NonCoIntgr_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const auto& inMat = inputData[0];

    const int nRows = static_cast<int>(inMat.NumRows());
    const int nCols = static_cast<int>(inMat.NumColumns());

    if (nRows <= 0 || nCols <= 0) return false;
    if (m_Number <= 0) return false;

    SystemVueModelBuilder::Matrix<double> outMat;

    // 情况 1：行方向为脉冲维
    if (nRows == m_Number)
    {
        outMat.Resize(1, nCols);
        for (int col = 0; col < nCols; ++col)
        {
            double sumAbs = 0.0;
            for (int row = 0; row < nRows; ++row)
                sumAbs += std::abs(inMat(row, col));
            outMat(0, col) = sumAbs;
        }
    }
    // 情况 2：列方向为脉冲维
    else if (nCols == m_Number)
    {
        outMat.Resize(nRows, 1);
        for (int row = 0; row < nRows; ++row)
        {
            double sumAbs = 0.0;
            for (int col = 0; col < nCols; ++col)
                sumAbs += std::abs(inMat(row, col));
            outMat(row, 0) = sumAbs;
        }
    }
    // 情况 3：行方向按 Number 分块
    else if ((nRows % m_Number) == 0)
    {
        const int outRows = nRows / m_Number;
        outMat.Resize(outRows, nCols);
        for (int row = 0; row < outRows; ++row)
        {
            for (int col = 0; col < nCols; ++col)
            {
                double sumAbs = 0.0;
                for (int pulse = 0; pulse < m_Number; ++pulse)
                    sumAbs += std::abs(inMat(pulse * outRows + row, col));
                outMat(row, col) = sumAbs;
            }
        }
    }
    // 情况 4：列方向按 Number 分块
    else if ((nCols % m_Number) == 0)
    {
        const int outCols = nCols / m_Number;
        outMat.Resize(nRows, outCols);
        for (int row = 0; row < nRows; ++row)
        {
            for (int col = 0; col < outCols; ++col)
            {
                double sumAbs = 0.0;
                for (int pulse = 0; pulse < m_Number; ++pulse)
                    sumAbs += std::abs(inMat(row, pulse * outCols + col));
                outMat(row, col) = sumAbs;
            }
        }
    }
    else
    {
        qDebug()<<"nRows"<<nRows<<"nCols"<<nCols<<"m_Number"<<m_Number;
        return false;
    }

    std::vector<SystemVueModelBuilder::Matrix<double>> outVec;
    outVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outVec);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_NonCoIntgr_M_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
        if (inputData.empty()) return true;
        m_inputBuffer.push_back(inputData[0]);
    }

    // ② 处理（rate=1，每来一个矩阵就处理）
    if (!m_inputBuffer.empty())
    {
        const auto& inMat = m_inputBuffer.front();
        const int nRows = static_cast<int>(inMat.NumRows());
        const int nCols = static_cast<int>(inMat.NumColumns());

        SystemVueModelBuilder::Matrix<double> outMat;

        bool valid = (nRows > 0 && nCols > 0 && m_Number > 0);
        if (valid)
        {
            if (nRows == m_Number)
            {
                outMat.Resize(1, nCols);
                for (int col = 0; col < nCols; ++col)
                {
                    double sumAbs = 0.0;
                    for (int row = 0; row < nRows; ++row)
                        sumAbs += std::abs(inMat(row, col));
                    outMat(0, col) = sumAbs;
                }
            }
            else if (nCols == m_Number)
            {
                outMat.Resize(nRows, 1);
                for (int row = 0; row < nRows; ++row)
                {
                    double sumAbs = 0.0;
                    for (int col = 0; col < nCols; ++col)
                        sumAbs += std::abs(inMat(row, col));
                    outMat(row, 0) = sumAbs;
                }
            }
            else if ((nRows % m_Number) == 0)
            {
                const int outRows = nRows / m_Number;
                outMat.Resize(outRows, nCols);
                for (int row = 0; row < outRows; ++row)
                {
                    for (int col = 0; col < nCols; ++col)
                    {
                        double sumAbs = 0.0;
                        for (int pulse = 0; pulse < m_Number; ++pulse)
                            sumAbs += std::abs(inMat(pulse * outRows + row, col));
                        outMat(row, col) = sumAbs;
                    }
                }
            }
            else if ((nCols % m_Number) == 0)
            {
                const int outCols = nCols / m_Number;
                outMat.Resize(nRows, outCols);
                for (int row = 0; row < nRows; ++row)
                {
                    for (int col = 0; col < outCols; ++col)
                    {
                        double sumAbs = 0.0;
                        for (int pulse = 0; pulse < m_Number; ++pulse)
                            sumAbs += std::abs(inMat(row, pulse * outCols + col));
                        outMat(row, col) = sumAbs;
                    }
                }
            }
        }

        m_outputQueue.push(outMat);
        m_inputBuffer.erase(m_inputBuffer.begin());
    }

    // ③ 出队写入
    if (!m_outputQueue.empty())
    {
        std::vector<SystemVueModelBuilder::Matrix<double>> outVec;
        outVec.push_back(m_outputQueue.front());
        WriteOutputData(GetOutputPortName(0), outVec);
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_NonCoIntgr_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_NonCoIntgr_M>();

    SetDefaultParameters();

    try { m_Number = std::stoi(getParameter("Number").Value); } catch (...) {}

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}
