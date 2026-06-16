#include "Add_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

// 初始化基类 Block，无额外初始化逻辑
Add_Block::Add_Block(const std::string& name)
	: Block(name)
{
}

// ============================================================================
// SetDefaultParamters — 参数默认值设置（当前无参数，预留扩展）
// ============================================================================

void Add_Block::SetDefaultParamters()
{
}

// ============================================================================
// DataStreamRun — 固定步长批量求和模式
// ============================================================================
//
// 读取输入端口的全部样本数据，累加求和后将单个结果写入输出端口。
//
// 输入端口类型为 DOUBLE_BUS（多路 double 总线），
// ReadInputData<double> 会展开所有总线通道的数据到一个一维向量中，
// 本函数对该向量中所有元素求和。

bool Add_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);   // 输入端口（多路 BUS）
    std::string outputPort = GetOutputPortName(0);  // 输出端口（单值求和结果）

    // 展开读取所有 BUS 通道的输入数据
    auto inputData = ReadInputData<double>(inputPort);

    if (inputData.empty()) {
        return false;
    }
    qDebug() << "Add_Block::Run-- inputData: " << inputData.size();

    // 累加所有输入数据元素
    double acc(0.0);
    for (size_t i = 0; i < inputData.size(); ++i) {
        acc += inputData[i];
    }

    // 将求和结果作为单个输出写入
    std::vector<double> outputData;
    outputData.push_back(acc);
    qDebug() << "Add_Block::Run-- outputData: " << outputData[0];
    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积求和模式
// ============================================================================
//
// 工作于变步长仿真模式（VariableStep），样本以非固定时间间隔到达。
// 处理流程分为三步：
//
//   1. 【累积】遍历 BUS 上的所有桥接 Reader（bridge reader），
//      将每个 Reader 到达的样本累积到 m_inputBuffer 中各自对应的缓冲区。
//
//   2. 【判断】检查是否所有输入通道的缓冲区都已累积了至少 1 个样本，
//      若未全部就绪则跳过本帧，等待下次 firing 继续累积。
//
//   3. 【求和输出】从每个通道缓冲区取出第一个样本，累加求和后
//      推入输出队列（FIFO），在后续 firing 中逐点写出。
//
// 注意：本函数每次只处理"每个通道 1 个样本"的配对求和，
//       即假设每次 firing 中所有通道会同步到达相同数量的样本。

bool Add_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);   // 输入端口（BUS）
    std::string outputPort = GetOutputPortName(0);  // 输出端口

    // —— 步骤 1：从 BUS 的各桥接 Reader 读取并累积样本 ——
    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    for (const auto& bridge_reader : bridge_readers) {
        std::vector<double> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if (inputData.empty()) {
            return true;   // 某通道无数据，等待下次 firing
        }
        // 将该通道到达的样本追加到对应缓冲区
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

    std::vector<double> outputData(1, 0.0);

    if (CanProcessData) {
        // —— 步骤 3：跨通道求和 ——
        // 从每个通道缓冲区取出第一个样本（索引 0），累加求和
        for (size_t i = 0; i < 1; ++i) {
            double sum = 0.0;

            for (auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
                sum += it->second[i];
            }

            outputData[i] = sum;
            // 将处理结果放入输出队列（FIFO），供后续 firing 逐点写出
            m_outputQueue.push(outputData[i]);
        }

        // 从输出队列取一个结果写出
        if (!m_outputQueue.empty()) {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<double>{outputValue});
            m_lastOutput = outputValue;   // 记录最后一次输出值（供外部查询）

            qDebug() << "[Add_Block] :" << m_outputCount
                     << " value:" << outputValue;

            // 清空所有通道的累积缓冲区，准备下一轮配对
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

// ============================================================================
// Setup — 仿真开始前重置
// ============================================================================

// 清空输出队列，避免上一轮仿真的残留数据影响当前结果
bool Add_Block::Setup()
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
bool Add_Block::Run()
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
//  - 输入端口（1 个）：DOUBLE_BUS 类型，支持多路 double 信号同时接入
//  - 输出端口（1 个）：CIRCULAR_BUFFER_DOUBLE 类型，输出所有输入的和
//  输出端口速率固定为 1（每次 firing 产生一个输出值）

bool Add_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	// 创建加法器算法实例
	m_add = std::make_unique<Add>();

	// 设置输出端口速率为 1
	m_add->output.SetRate(1U);

	// 注册输入端口（DOUBLE_BUS：多路 double 输入总线）
	AddInputPort("input", m_add->input, 1, Block::DataType::DOUBLE_BUS);
	// 注册输出端口（CIRCULAR_BUFFER_DOUBLE：单个 double 求和结果）
	AddOutputPort("output", m_add->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

	return true;
}
