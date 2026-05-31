#include "Reverse_Block.h"

Reverse_Block::Reverse_Block(const std::string &name)
    :Block(name)
    , m_N(64)
    , m_lastOutput(0.0)
    , m_inputCount(0)
    , m_outputCount(0)
    , m_samplePeriod(0.0)
{
}

bool Reverse_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

bool Reverse_Block::Run()
{
    // ========== 变步长模式 ==========
    if (IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool Reverse_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_reverse = std::make_unique<Reverse>();

    SetDefaultParameters();

    m_N = std::stoi(getParameter("N").Value);

    SetParameter(m_N);

    AddInputPort("input", m_reverse->input, static_cast<size_t>(m_N),
                 Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_reverse->output, static_cast<size_t>(m_N),
                  Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    //设置抽取因子
    SetDecimationFactor(m_N);

    return true;
}

void Reverse_Block::SetDefaultParameters()
{
    m_N = 64;
}

void Reverse_Block::SetParameter(int n)
{
    m_N = n;
    if (m_reverse) {
        m_reverse->N = m_N;
    }
}

// ========== 变步长接口实现 ==========

double Reverse_Block::GetMinimumTimeStep() const
{
    if (IsVariableStepMode() && m_samplePeriod > 0) {
        return m_samplePeriod;  // 不能小于累积周期
    }
    return 0.0;
}

bool Reverse_Block::ShouldExecuteAt(double time) const
{
    if (IsVariableStepMode()) {
        return time >= GetNextExecutionTime();
    }
    return true;  // N==1 时每步都执行
}

int Reverse_Block::GetOutputDataCount() const
{
    if (IsVariableStepMode()) {
        return m_N;  // 每次处理产出N个数据
    }
    return 1;
}

int Reverse_Block::GetInputAccumulateCount() const
{
    if (IsVariableStepMode()) {
        return m_N;  // 需要累积N个输入
    }
    return 1;
}

bool Reverse_Block::DataStreamRun()
{
    std::string InputPort = GetInputPortName(0);
    std::string OutputPort = GetOutputPortName(0);
    // ========== 数据流模式 ==========
    auto InputData = ReadInputData<double>(InputPort);
    size_t N = InputData.size();
    std::vector<double> OutputData;
    OutputData.reserve(N);
    for(size_t i = 0; i < N; i++) {
        OutputData.push_back(InputData[N - i - 1]);
    }
    WriteOutputData(OutputPort, OutputData);
    m_lastOutput = OutputData.empty() ? m_lastOutput : OutputData[0];

    return true;
}

bool Reverse_Block::TimeDrivenRun()
{
    std::string InputPort = GetInputPortName(0);
    std::string OutputPort = GetOutputPortName(0);
    // ----- 步骤1: 读取输入数据并累积 -----
    auto InputData = ReadInputData<double>(InputPort);
    if(InputData.empty()) return true;


    // 将新数据加入累积缓冲区
    for (size_t i = 0; i < InputData.size(); i++) {
            m_inputBuffer.push_back(InputData[i]);
    }

    // ----- 步骤2: 检查是否累积够N个，执行处理 -----
    // 对累积的N个数据进行反转处理
    std::vector<double> outputData;
    outputData.reserve(m_N);

    for (int i = 0; i < m_N; i++) {
        outputData.push_back(m_inputBuffer[m_N - i - 1]);
    }

    // 将处理结果放入输出队列
    for (int i = 0; i < m_N; i++) {
        m_outputQueue.push(outputData[i]);
    }

    // ----- 步骤3: 从输出队列取一个数据输出 -----
    if (!m_outputQueue.empty()) {
        double outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(OutputPort, std::vector<double>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[Reverse_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
        return true;
    }
    return true;
}
