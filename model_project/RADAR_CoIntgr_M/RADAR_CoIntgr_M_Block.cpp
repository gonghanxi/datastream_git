#include "RADAR_CoIntgr_M_Block.h"

#include <complex>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_CoIntgr_M_Block::RADAR_CoIntgr_M_Block(const std::string& name)
    : Block(name)
    , m_NumOfPulse(32)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_CoIntgr_M_Block::SetDefaultParameters()
{
    m_NumOfPulse = 32;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_CoIntgr_M_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->NumOfPulse = m_NumOfPulse;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_CoIntgr_M_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    if (m_NumOfPulse <= 0) {
        LOG_ERROR("RADAR_CoIntgr_M: NumOfPulse must be greater than 0.");
        return false;
    }

    SetParameters();
    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_CoIntgr_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// processOneMatrix —— 相参积累核心处理（移植自 RADAR_CoIntgr_M::Run）
// ============================================================================

namespace {

bool processOneMatrix(
    const SystemVueModelBuilder::DComplexMatrix& inMat,
    SystemVueModelBuilder::DComplexMatrix& outMat,
    int numOfPulse)
{
    const int totalNum = static_cast<int>(inMat.NumElements());

    if (totalNum <= 0) {
        LOG_ERROR("RADAR_CoIntgr_M: input matrix must contain at least one element.");
        return false;
    }

    if (numOfPulse <= 0) {
        LOG_ERROR("RADAR_CoIntgr_M: NumOfPulse must be greater than 0.");
        return false;
    }
    qDebug()<<"numOfPulse"<<numOfPulse<<"totalNum"<<totalNum;

    if ((totalNum % numOfPulse) != 0) {
        LOG_ERROR("RADAR_CoIntgr_M: input matrix element count must be an integer multiple of NumOfPulse.");
        return false;
    }

    const int PRN = totalNum / numOfPulse;

    outMat.Resize(1, PRN);

    for (int i = 0; i < PRN; ++i) {
        outMat(i).real(0.0);
        outMat(i).imag(0.0);

        for (int pulseIndex = 0; pulseIndex < numOfPulse; ++pulseIndex) {
            outMat(i) += inMat(pulseIndex * PRN + i);
        }
    }

    return true;
}

} // namespace

// ============================================================================
// DataStreamRun — 数据流模式：一次处理整帧矩阵
// ============================================================================

bool RADAR_CoIntgr_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    SystemVueModelBuilder::DComplexMatrix outMat;
    if (!processOneMatrix(inputData[0], outMat, m_NumOfPulse)) {
        return false;
    }

    std::vector<SystemVueModelBuilder::DComplexMatrix> outVec;
    outVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outVec);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_CoIntgr_M_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    // ① 累积输入矩阵
    m_inputBuffer.push_back(inputData[0]);

    // ② 每个矩阵独立处理（rate=1 每次一个完整矩阵）
    if (!m_inputBuffer.empty()) {
        SystemVueModelBuilder::DComplexMatrix outMat;
        if (!processOneMatrix(m_inputBuffer.back(), outMat, m_NumOfPulse)) {
            m_inputBuffer.pop_back();
            return false;
        }
        m_outputQueue.push(outMat);
        m_inputBuffer.pop_back();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty()) {
        std::vector<SystemVueModelBuilder::DComplexMatrix> outVec;
        outVec.push_back(m_outputQueue.front());
        WriteOutputData(GetOutputPortName(0), outVec);
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_CoIntgr_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_CoIntgr_M>();

    SetDefaultParameters();

    try { m_NumOfPulse = std::stoi(getParameter("NumOfPulse").Value); } catch (...) {}

    if (m_NumOfPulse <= 0) {
        LOG_ERROR("RADAR_CoIntgr_M: NumOfPulse must be greater than 0.");
        return false;
    }

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}
