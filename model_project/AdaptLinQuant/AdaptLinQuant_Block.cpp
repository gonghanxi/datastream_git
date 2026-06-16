#include "AdaptLinQuant_Block.h"

#include <cmath>

// ============================================================================
// 构造函数
// ============================================================================

// 初始化基类 Block 并设置量化位数的默认值（8 位）
AdaptLinQuant_Block::AdaptLinQuant_Block(const std::string& name)
    : Block(name)
    , m_Bits(8)
{
}

// ============================================================================
// Setup — 仿真开始前重置所有累计队列
// ============================================================================

// 每次进入新一轮仿真时清空 TimeDrivenRun 中使用的输出队列，
// 避免上一轮残留的数据影响当前结果
bool AdaptLinQuant_Block::Setup()
{
    Block::Setup();
    m_amplitudeQueue = std::queue<double>();   // 量化幅值输出队列
    m_outStepQueue   = std::queue<double>();   // 输出步长队列
    m_stepLevelQueue = std::queue<int>();      // 量化级数（索引）队列
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

// 根据仿真模式分发到不同的执行路径：
//  - 变步长模式（VariableStep / Time-Driven）：逐点累积处理，走 TimeDrivenRun
//  - 固定步长模式（DataStream / Fixed-Rate）：批量处理，走 DataStreamRun
bool AdaptLinQuant_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 固定步长批量量化模式
// ============================================================================
//
// 每次调用对当前输入端口的第一个样本执行自适应线性量化。
//
// 量化算法：
//   设 L = 2^Bits（量化级数），量化步长为 step。
//   输入信号 x 的量化索引 k 由下式决定：
//     k_raw = x / step + (L/2 - 0.5) + 0.5  ← 加 0.5 实现四舍五入取整
//     k = clamp( floor(k_raw),  0,  L-1 )
//
//   量化输出值（去归一化反量化）：
//     q = (k - (L/2 - 0.5)) * step
//
//   该量化器为 mid-tread 型均匀量化器，量化电平数为 2^Bits，
//   量化范围中心在 0，适合对称分布的信号。

bool AdaptLinQuant_Block::DataStreamRun()
{
    // 获取端口名称（2 输入 + 3 输出）
    std::string inputPort    = GetInputPortName(0);   // 输入信号
    std::string inStepPort   = GetInputPortName(1);   // 输入量化步长
    std::string amplitudePort = GetOutputPortName(0);  // 量化后幅值
    std::string outStepPort   = GetOutputPortName(1);  // 输出步长（透传）
    std::string stepLevelPort = GetOutputPortName(2);  // 量化级数索引

    // 读取输入数据（每次 firing 读取一批）
    auto inputData  = ReadInputData<double>(inputPort);
    auto inStepData = ReadInputData<double>(inStepPort);

    if (inputData.empty() || inStepData.empty()) return true;

    // 取当前量化步长（第一个样本的步长作为本次量化的步长）
    const double step = inStepData[0];

    // 步长有效性检查：必须为正有限值
    if (!(step > 0.0) || !std::isfinite(step))
    {
        LOG_ERROR("AdaptLinQuant: inStep must be finite and > 0.");
        return false;
    }

    // 量化级数 L = 2^Bits，例如 8 位 → L = 256
    const unsigned int L_u = (1u << m_Bits);
    const double       L   = static_cast<double>(L_u);
    // 半跨度偏移量：(L/2 - 0.5)，用于将量化区间中心对齐到 0
    const double halfSpanMinusHalf = 0.5 * L - 0.5;

    // 取输入信号的第一个样本
    const double x = inputData[0];

    // 计算量化索引（浮点 → 四舍五入取整 → 整数索引）
    // kReal = x / step + (L/2 - 0.5)
    // kRound = floor(kReal + 0.5)  ← 四舍五入
    double kRound = std::floor(x / step + halfSpanMinusHalf + 0.5);

    // 钳位到有效量化级数范围 [0, L-1]
    if (kRound < 0.0)
        kRound = 0.0;

    const double kMax = static_cast<double>(L_u - 1u);
    if (kRound > kMax)
        kRound = kMax;

    const int k = static_cast<int>(kRound);

    // 去归一化：将量化索引 k 映射回实际幅值
    // q = (k - halfSpanMinusHalf) * step
    // 例如 step=0.1, Bits=8, k=127 → q ≈ 0
    const double q = (static_cast<double>(k) - halfSpanMinusHalf) * step;

    // 构建输出向量并写入端口
    std::vector<double> amplitudeData;
    std::vector<double> outStepData;
    std::vector<int>    stepLevelData;

    amplitudeData.push_back(q);      // 输出：量化后幅值
    outStepData.push_back(step);     // 输出：步长（透传）
    stepLevelData.push_back(k);      // 输出：量化级数索引

    WriteOutputData(amplitudePort, amplitudeData);
    WriteOutputData(outStepPort,   outStepData);
    WriteOutputData(stepLevelPort, stepLevelData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积模式
// ============================================================================
//
// 与 DataStreamRun 不同，TimeDrivenRun 工作于变步长仿真模式下。
// 输入样本以非固定时间间隔到达，因此需要：
//   1. 将每次到达的样本累积到内部缓冲区（m_inputBuffer / m_inStepBuffer）
//   2. 当两个输入缓冲区都至少有 1 个样本时，触发一次量化计算
//   3. 量化结果推入输出队列（FIFO），在后续 firing 中逐点写出
//   4. 计算完成后清空输入缓冲区，等待下一对样本
//
// 量化算法与 DataStreamRun 完全相同（mid-tread 均匀量化器）。

bool AdaptLinQuant_Block::TimeDrivenRun()
{
    // 获取端口名称
    std::string inputPort    = GetInputPortName(0);   // 输入信号
    std::string inStepPort   = GetInputPortName(1);   // 输入量化步长
    std::string amplitudePort = GetOutputPortName(0);  // 量化后幅值（输出队列）
    std::string outStepPort   = GetOutputPortName(1);  // 输出步长（输出队列）
    std::string stepLevelPort = GetOutputPortName(2);  // 量化级数索引（输出队列）

    auto inputData  = ReadInputData<double>(inputPort);
    auto inStepData = ReadInputData<double>(inStepPort);

    // 步骤一：将本帧到达的样本累积到输入缓冲区
    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < inStepData.size(); ++i)
        m_inStepBuffer.push_back(inStepData[i]);

    // 步骤二：当两个输入缓冲区各至少有 1 个样本时，执行一次量化
    if (static_cast<int>(m_inputBuffer.size()) >= 1
        && static_cast<int>(m_inStepBuffer.size()) >= 1)
    {
        // 取第一个累积的步长值作为本次量化的步长
        const double step = m_inStepBuffer[0];

        // 步长有效性检查
        if (!(step > 0.0) || !std::isfinite(step))
        {
            LOG_ERROR("AdaptLinQuant: inStep must be finite and > 0.");
            return false;
        }

        // 量化级数和半跨度偏移（与 DataStreamRun 相同）
        const unsigned int L_u = (1u << m_Bits);
        const double       L   = static_cast<double>(L_u);
        const double halfSpanMinusHalf = 0.5 * L - 0.5;

        // 取第一个累积的输入样本
        const double x = m_inputBuffer[0];

        // 量化索引计算：四舍五入 + 钳位到 [0, L-1]
        double kRound = std::floor(x / step + halfSpanMinusHalf + 0.5);

        if (kRound < 0.0)
            kRound = 0.0;

        const double kMax = static_cast<double>(L_u - 1u);
        if (kRound > kMax)
            kRound = kMax;

        const int k = static_cast<int>(kRound);

        // 去归一化：得到量化幅值
        const double q = (static_cast<double>(k) - halfSpanMinusHalf) * step;

        // 将量化结果推入输出队列（FIFO），等待逐点写出
        m_amplitudeQueue.push(q);
        m_outStepQueue.push(step);
        m_stepLevelQueue.push(k);

        // 清空输入缓冲区，准备累积下一对样本
        m_inputBuffer.clear();
        m_inStepBuffer.clear();
    }

    // 步骤三：逐点写出输出队列中的结果（每次 firing 最多各写一个）
    if (!m_amplitudeQueue.empty())
    {
        std::vector<double> amplitudeData;
        amplitudeData.push_back(m_amplitudeQueue.front());
        m_amplitudeQueue.pop();
        WriteOutputData(amplitudePort, amplitudeData);
    }
    if (!m_outStepQueue.empty())
    {
        std::vector<double> outStepData;
        outStepData.push_back(m_outStepQueue.front());
        m_outStepQueue.pop();
        WriteOutputData(outStepPort, outStepData);
    }
    if (!m_stepLevelQueue.empty())
    {
        std::vector<int> stepLevelData;
        stepLevelData.push_back(m_stepLevelQueue.front());
        m_stepLevelQueue.pop();
        WriteOutputData(stepLevelPort, stepLevelData);
    }

    return true;
}

// ============================================================================
// Initialize — 模块初始化
// ============================================================================
//
// 解析用户参数并注册输入/输出端口：
//  - 参数：Bits（量化位数，范围 1~31，默认 8）
//  - 输入端口（2 个）：input（待量化信号）、inStep（量化步长）
//  - 输出端口（3 个）：amplitude（量化幅值）、outStep（步长透传）、stepLevel（量化级数索引）
//  所有端口速率均为 1，数据类型为 CIRCULAR_BUFFER。

bool AdaptLinQuant_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    // 创建算法类实例（AdaptLinQuant 定义端口容器，但不参与实际计算）
    m_AdaptLinQuant = std::make_unique<AdaptLinQuant>();

    // 解析 Bits 参数，解析失败则使用默认值 8
    try
    {
        m_Bits = std::stoi(getParameter("Bits").Value);
    }
    catch (...) { LOG_WARN("Failed to parse parameter 'Bits', using default value."); }

    // Bits 参数合法性检查：必须介于 1 到 31 之间
    if (m_Bits < 1 || m_Bits > 31)
    {
        LOG_ERROR("AdaptLinQuant: Bits must be between 1 and 31.");
        return false;
    }

    // 注册端口：所有端口速率 = 1 根据原算法的setrate函数设置的值,没设置默认为1
    AddInputPort("input",   m_AdaptLinQuant->input,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);   // 输入信号
    AddInputPort("inStep",  m_AdaptLinQuant->inStep, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);   // 量化步长
    AddOutputPort("amplitude", m_AdaptLinQuant->amplitude, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE); // 量化幅值
    AddOutputPort("outStep",   m_AdaptLinQuant->outStep,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE); // 步长（透传）
    AddOutputPort("stepLevel", m_AdaptLinQuant->stepLevel, 1, Block::DataType::CIRCULAR_BUFFER_INT);    // 量化级数索引

    return true;
}
