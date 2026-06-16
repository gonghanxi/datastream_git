#include "AddCx_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

// 初始化基类 Block，无额外初始化逻辑
AddCx_Block::AddCx_Block(const std::string& name)
    :Block(name)
{

}

// ============================================================================
// Setup — 仿真开始前重置
// ============================================================================

// 清空输出队列，避免上一轮仿真的残留数据影响当前结果
bool AddCx_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

// 根据仿真模式分发：
//  - 变步长模式（VariableStep）→ TimeDrivenRun（逐点累积求和）
//  - 固定步长模式（DataStream）  → DataStreamRun（批量求和）
bool AddCx_Block::Run()
{
    if (IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

// ============================================================================
// Initialize — 模块初始化
// ============================================================================
//
// 创建算法实例并注册端口：
//  - 输入端口（1 个）：DCOMPLEX_BUS 类型，支持多路复数信号同时接入
//  - 输出端口（1 个）：CIRCULAR_BUFFER_DCOMPLEX 类型，输出所有输入的复数和
//  输出端口速率固定为 1，每次 firing 产生一个输出值

bool AddCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    // 创建复数加法器算法实例
    m_addCx = std::make_unique<AddCx>();

    // 注册输入端口（DCOMPLEX_BUS：多路复数输入总线）
    AddInputPort("input", m_addCx->input, 1, DataType::DCOMPLEX_BUS);
    // 注册输出端口（CIRCULAR_BUFFER_DCOMPLEX：单个复数求和结果）
    AddOutputPort("output", m_addCx->output, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    // 设置输出端口每次写入大小为 1
    GetOutputPort(GetOutputPortName(0))->SetWriteSize(1U);

    return true;
}

// ============================================================================
// DataStreamRun — 固定步长批量复数求和模式
// ============================================================================
//
// 读取输入端口的全部复数样本数据，累加求和后将单个复数结果写入输出端口。
//
// 输入端口类型为 DCOMPLEX_BUS（多路 complex<double> 总线），
// ReadInputData<complex<double>> 会展开所有总线通道的数据到一个一维向量中，
// 本函数对该向量中所有复数元素求和（实部与实部相加，虚部与虚部相加）。

bool AddCx_Block::DataStreamRun()
{
    std::string inputPortName  = GetInputPortName(0);   // 输入端口（多路复数 BUS）
    // 展开读取所有 BUS 通道的复数输入数据
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPortName);

    // 累加所有复数输入数据元素
    // 复数加法：实部 + 实部，虚部 + 虚部
    std::complex<double> acc(0.0, 0.0);
    for (size_t i = 0; i < inputData.size(); ++i) {
        acc += inputData[i];
    }

    // 将复数求和结果作为单个输出写入
    std::string outputPortName = GetOutputPortName(0);
    std::vector<std::complex<double>> outputData;
    outputData.push_back(acc);

    WriteOutputData(outputPortName, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积复数求和模式
// ============================================================================
//
// 工作于变步长仿真模式（VariableStep），样本以非固定时间间隔到达。
// 处理流程分为三步：
//
//   1. 【累积】遍历 BUS 上的所有桥接 Reader（bridge reader），
//      将每个 Reader 到达的复数样本累积到 m_inputBuffer 中各自对应的缓冲区。
//
//   2. 【判断】检查是否所有输入通道的缓冲区都已累积了至少 1 个样本，
//      若未全部就绪则跳过本帧，等待下次 firing 继续累积。
//
//   3. 【求和输出】从每个通道缓冲区取出第一个样本，复数累加求和后
//      推入输出队列（FIFO），在后续 firing 中逐点写出。
//
// 注意：本函数每次只处理"每个通道 1 个样本"的配对求和，
//       即假设每次 firing 中所有通道会同步到达相同数量的样本。

bool AddCx_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);   // 输入端口（复数 BUS）
    std::string outputPort = GetOutputPortName(0);  // 输出端口

    // —— 步骤 1：从 BUS 的各桥接 Reader 读取并累积复数样本 ——
    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    for (const auto& bridge_reader : bridge_readers) {
        std::vector<std::complex<double>> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if (inputData.empty()) {
            return true;   // 某通道无数据，等待下次 firing
        }
        // 将该通道到达的复数样本追加到对应缓冲区
        for (size_t i = 0; i < inputData.size(); i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    // —— 步骤 2：检查是否所有通道都至少累积了 1 个样本 ——
    // 若任一通道缓冲区为空，则 CanProcessData 保持 true，跳过本次处理
    bool CanProcessData = true;
    for (auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if (it->second.size() >= 1) {
            CanProcessData = false;   // 标记为"不可处理"（等待更多数据）
            break;
        }
    }

    std::vector<std::complex<double>> outputData(1, 0.0);

    if (CanProcessData) {
        // —— 步骤 3：跨通道复数求和 ——
        // 从每个通道缓冲区取出第一个样本（索引 0），复数累加求和
        for (size_t i = 0; i < 1; ++i) {
            std::complex<double> sum(0.0, 0.0);

            // 遍历每个缓冲区，累加第 i 个元素（复数加法）
            for (auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                sum += it->second[i];
            }

            outputData[i] = sum;
            // 将处理结果放入输出队列（FIFO），供后续 firing 逐点写出
            m_outputQueue.push(outputData[i]);
        }

        // 从输出队列取一个结果写出
        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;   // 记录最后一次输出值（供外部查询）

            qDebug() << "[AddCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();

            // 清空所有通道的累积缓冲区，准备下一轮配对
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}
