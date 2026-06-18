#include "RADAR_DBF_Block.h"
#include <algorithm>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_DBF_Block::RADAR_DBF_Block(const std::string& name)
    :Block(name)
{

}

// ============================================================================
// Setup — 仿真开始前重置
// ============================================================================

bool RADAR_DBF_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

bool RADAR_DBF_Block::Run()
{
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

// ============================================================================
// Initialize — 模块初始化
// ============================================================================
//
// 创建算法实例并注册端口：
//  - 输入端口（2 个）：
//      input  : DCOMPLEX_BUS 类型，多路复数信号（各通道/阵元）
//      weight : DCOMPLEX_BUS 类型，多路复数权值（波束形成权重）
//  - 输出端口（1 个）：
//      output : CIRCULAR_BUFFER_DCOMPLEX 类型，加权求和后的单路复数输出
//  输出端口速率固定为 1，每次 firing 产生一个输出值

bool RADAR_DBF_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_dbf = std::make_unique<RADAR_DBF>();

    // 注册输入端口（DCOMPLEX_BUS：多路复数输入总线）
    AddInputPort("input", m_dbf->input, 1, DataType::DCOMPLEX_BUS);
    AddInputPort("weight", m_dbf->weight, 1, DataType::DCOMPLEX_BUS);
    // 注册输出端口（CIRCULAR_BUFFER_DCOMPLEX：加权求和后的单个复数结果）
    AddOutputPort("output", m_dbf->output, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    // 设置输出端口每次写入大小为 1
    GetOutputPort(GetOutputPortName(0))->SetWriteSize(1U);

    return true;
}

// ============================================================================
// DataStreamRun — 固定步长数字波束形成模式
// ============================================================================
//
// 读取 input 和 weight 两个 BUS 端口的全部复数样本数据，
// 逐通道配对相乘后累加求和：output = sum(input[k] * weight[k])
// 将单个复数结果写入输出端口。
//
// ReadInputData<complex<double>> 会展开所有总线通道的数据到一个一维向量中，
// 两个 BUS 的通道数应一致（由原始算法 validateAndPrepare_ 保证）。

bool RADAR_DBF_Block::DataStreamRun()
{
    std::string inputPortName  = GetInputPortName(0);   // input bus（多路复数 BUS）
    std::string weightPortName = GetInputPortName(1);   // weight bus（多路复数 BUS）
    std::string outputPortName = GetOutputPortName(0);  // 输出端口

    // 展开读取所有 BUS 通道的复数数据
    std::vector<std::complex<double>> inputData  = ReadInputData<std::complex<double>>(inputPortName);
    std::vector<std::complex<double>> weightData = ReadInputData<std::complex<double>>(weightPortName);

    if(inputData.empty() || weightData.empty()) {
        std::vector<std::complex<double>> outputData;
        outputData.push_back(std::complex<double>(0.0, 0.0));
        WriteOutputData(outputPortName, outputData);
        return true;
    }

    // 逐通道配对相乘并累加
    std::complex<double> acc(0.0, 0.0);
    int busSize = static_cast<int>(std::min(inputData.size(), weightData.size()));
    for(int k = 0; k < busSize; ++k) {
        acc += inputData[k] * weightData[k];
    }

    // 将波束形成结果作为单个输出写入
    std::vector<std::complex<double>> outputData;
    outputData.push_back(acc);
    WriteOutputData(outputPortName, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积波束形成模式
// ============================================================================
//
// 工作于变步长仿真模式（VariableStep），样本以非固定时间间隔到达。
// 处理流程分为三步：
//
//   1. 【累积】分别遍历 input bus 和 weight bus 上的所有桥接 Reader，
//      将每个 Reader 到达的复数样本累积到各自对应的缓冲区中。
//
//   2. 【判断】检查两个 bus 的所有通道缓冲区是否都已累积了至少 1 个样本，
//      若未全部就绪则跳过本帧，等待下次 firing 继续累积。
//
//   3. 【波束形成输出】从每个通道缓冲区取出第一个样本，
//      逐通道配对相乘后累加求和，推入输出队列（FIFO），
//      在后续 firing 中逐点写出。

bool RADAR_DBF_Block::TimeDrivenRun()
{
    std::string inputPortName  = GetInputPortName(0);   // input bus（复数 BUS）
    std::string weightPortName = GetInputPortName(1);   // weight bus（复数 BUS）
    std::string outputPortName = GetOutputPortName(0);  // 输出端口

    // —— 步骤 1a：从 input BUS 的各桥接 Reader 读取并累积复数样本 ——
    BufferReader* inputMaster = GetInputPort(inputPortName);
    auto inputBridgeReaders = inputMaster->GetBusConnections();
    for(const auto& conn : inputBridgeReaders) {
        std::vector<std::complex<double>> channelData;
        conn.bridgeReader->ReadData(channelData);
        if(channelData.empty()) {
            return true;   // 某通道无数据，等待下次 firing
        }
        for(size_t i = 0; i < channelData.size(); i++) {
            m_inputBuffer[conn.bridgeReader].push_back(channelData[i]);
        }
    }

    // —— 步骤 1b：从 weight BUS 的各桥接 Reader 读取并累积复数样本 ——
    BufferReader* weightMaster = GetInputPort(weightPortName);
    auto weightBridgeReaders = weightMaster->GetBusConnections();
    for(const auto& conn : weightBridgeReaders) {
        std::vector<std::complex<double>> channelData;
        conn.bridgeReader->ReadData(channelData);
        if(channelData.empty()) {
            return true;   // 某通道无数据，等待下次 firing
        }
        for(size_t i = 0; i < channelData.size(); i++) {
            m_weightBuffer[conn.bridgeReader].push_back(channelData[i]);
        }
    }

    // —— 步骤 2：检查两个 bus 的所有通道是否都至少累积了 1 个样本 ——
    bool canProcessData = true;
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() < 1) {
            canProcessData = false;
            break;
        }
    }
    if(canProcessData) {
        for(auto it = m_weightBuffer.begin(); it != m_weightBuffer.end(); ++it) {
            if(it->second.size() < 1) {
                canProcessData = false;
                break;
            }
        }
    }

    if(canProcessData) {
        // —— 步骤 3：逐通道配对相乘并累加求和 ——
        // 将两个 map 按插入顺序配对（bus 通道顺序一致）
        std::complex<double> acc(0.0, 0.0);
        auto inputIt  = m_inputBuffer.begin();
        auto weightIt = m_weightBuffer.begin();
        while(inputIt != m_inputBuffer.end() && weightIt != m_weightBuffer.end()) {
            acc += inputIt->second[0] * weightIt->second[0];
            ++inputIt;
            ++weightIt;
        }

        m_outputQueue.push(acc);

        // 从输出队列取一个结果写出
        if(!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPortName, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[RADAR_DBF_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();

            // 清空所有通道的累积缓冲区，准备下一轮配对
            m_inputBuffer.clear();
            m_weightBuffer.clear();
            return true;
        }
    }
    return true;
}
