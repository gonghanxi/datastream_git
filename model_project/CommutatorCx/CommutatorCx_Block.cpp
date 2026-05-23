#include "CommutatorCx_Block.h"
#include <vector>

CommutatorCx_Block::CommutatorCx_Block(const std::string& name)
    : Block(name)
{
}

void CommutatorCx_Block::SetDefaultParamters()
{
    m_blockSize = 1;
    m_iBlockSize = 1U;
}

bool CommutatorCx_Block::ModelSetup()
{
    m_iBlockSize = (m_blockSize < 1) ? 1U : static_cast<size_t>(m_blockSize);

    auto* inputPortObj = GetInputPort(GetInputPortName(0));
    auto* outputPortObj = GetOutputPort(GetOutputPortName(0));

    const size_t numInputs = inputPortObj ? inputPortObj->GetBusConnectionCount() : 0U;

    if (inputPortObj) {
        inputPortObj->SetReadSize(m_iBlockSize);
    }
    if (outputPortObj) {
        outputPortObj->SetWriteSize(m_iBlockSize * numInputs);
    }
    return true;
}

void CommutatorCx_Block::SetParameters()
{
    if (!m_commutatorCx) {
        return;
    }

    m_commutatorCx->BlockSize = m_blockSize;
    m_commutatorCx->m_iBlockSize = m_iBlockSize;
}

bool CommutatorCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool CommutatorCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CommutatorCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_commutatorCx = std::make_unique<CommutatorCx>();

    AddInputPort("input", m_commutatorCx->input, 1, Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_commutatorCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_blockSize = std::stoi(getParameter("BlockSize").Value); } catch (...) { }

    SetParameters();

    return true;
}

bool CommutatorCx_Block::DataStreamRun()
{
    const std::string inputPortName = GetInputPortName(0);
    const std::string outputPortName = GetOutputPortName(0);

    auto* inputPort = GetInputPort(inputPortName);
    auto connections = inputPort->GetBusConnections();
    if (connections.empty()) return false;

    const size_t blockSize = m_iBlockSize;
    std::vector<std::complex<double>> outputData;
    outputData.reserve(blockSize * connections.size());

    for (auto& conn : connections)
    {
        std::vector<std::complex<double>> channelData;
        conn.bridgeReader->ReadData(channelData);
        if (channelData.size() < blockSize)
        {
            // 固定步长模式下，预期每个子端口至少提供 blockSize 个样本
            // 若不足，则用0填充并警告
            LOG_WARN("Commutator: insufficient data on input channel, padding with zeros");
            channelData.resize(blockSize, 0.0);
        }
        outputData.insert(outputData.end(), channelData.begin(), channelData.begin() + blockSize);
    }

    WriteOutputData(outputPortName, outputData);
    return true;
}

bool CommutatorCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();
    size_t maxWriteSize = 0;

    for(const auto& bridge_reader : bridge_readers) {
        maxWriteSize = std::max(maxWriteSize, bridge_reader.bridgeReader->GetConnectedBuffer()->GetWriteSize());

        std::vector<std::complex<double>> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if(inputData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < inputData.size();i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    bool CanProcessData = true;
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() != maxWriteSize) {
            CanProcessData = false;
            break;
        }
    }

    std::vector<std::complex<double>> outputData(m_iBlockSize * bridge_readers.size());  // 初始化为0
    // 步骤3：如果条件满足，则从每个子端口取出 m_iBlockSize 个样本，拼接成一个输出块，并压入输出队列
    if (CanProcessData)
    {
        size_t blockSize = m_iBlockSize;
        size_t numInputs = bridge_readers.size();
        std::vector<std::complex<double>> outputBlock;
        outputBlock.reserve(blockSize * numInputs);

        for (auto& conn : bridge_readers)
        {
            auto& buf = m_inputBuffer[conn.bridgeReader];
            // 取出前 blockSize 个样本
            outputBlock.insert(outputBlock.end(), buf.begin(), buf.begin() + blockSize);
            // 移除已取出的样本
            buf.erase(buf.begin(), buf.begin() + blockSize);
        }
        // 将输出块中的每个样本逐个放入输出队列
        for (const auto& val : outputBlock)
        {
            m_outputQueue.push(val);
        }
    }


    if (!m_outputQueue.empty())
    {
        std::complex<double> outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
        m_lastOutput = outputValue;
        m_inputBuffer.clear();

        qDebug() << "[Commutator_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << "," << outputValue.imag();
    }
    else
    {
        // 无输出时，可选择保持上一次输出（框架通常会保留上次数据）
        qDebug() << "[Commutator_Block] 无输出，保持上次值:" << m_lastOutput.real() << "," << m_lastOutput.imag();
    }

    return true;
}
