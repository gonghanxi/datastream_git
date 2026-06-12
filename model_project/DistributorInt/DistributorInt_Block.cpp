#include "DistributorInt_Block.h"

DistributorInt_Block::DistributorInt_Block(const std::string &name)
    :Block(name)
{

}
bool DistributorInt_Block::Setup()
{
    Block::Setup();
    if(!ModelsSetup()) return false;
    return true;
}

bool DistributorInt_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    // 读取所有输入数据
    std::vector<int> inputData = ReadInputData<int>(inputPort);

    // 获取输出 Buffer 和总线连接数
    Buffer* outputBuffer = GetOutputPort(outputPort);
    if (!outputBuffer) {
        LOG_ERROR("AsyncDistributor: Failed to get output buffer");
        return false;
    }

    const size_t numOutputs = outputBuffer->GetBusConnectionCount();
    if (numOutputs == 0 || m_iBlockSize == 0) {
        return true;
    }

    size_t k = 0;

    // 为每个输出通道准备数据并写入
    for (size_t i = 0; i < numOutputs; ++i) {
        // 提取当前通道的数据块
        std::vector<int> channelData;
        channelData.reserve(static_cast<size_t>(m_iBlockSize));

        for (size_t j = 0; j < m_iBlockSize; ++j) {
            if (k + j < inputData.size()) {
                channelData.push_back(inputData[k + j]);
            }
        }
        outputBuffer->WriteDataToChannel(static_cast<int>(i), channelData);

        k += static_cast<size_t>(m_iBlockSize);
    }
    return true;
}

bool DistributorInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Distributor = std::make_unique<DistributorInt>();

    SetDefaultParameters();

    try { m_BlockSize = std::stoi(getParameter("BlockSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BlockSize', using default value."); }

    SetParameters();

    AddInputPort("input", m_Distributor->input, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_Distributor->output, 1, DataType::INT_BUS);
    return true;
}

void DistributorInt_Block::SetParameters()
{
    if(!m_Distributor) return;
    m_Distributor->BlockSize = m_BlockSize;
}

void DistributorInt_Block::SetDefaultParameters()
{
    m_BlockSize = 1;
}

bool DistributorInt_Block::ModelsSetup()
{
    Buffer* outputBuffer = GetOutputPort(GetOutputPortName(0));
    BufferReader* inputReader = GetInputPort(GetInputPortName(0));
    if (m_BlockSize < 1)
        m_iBlockSize = 1;
    else
        m_iBlockSize = static_cast<size_t>(m_BlockSize);

    const size_t numOutputs = outputBuffer->GetBusConnectionCount();

    outputBuffer->SetWriteSize(static_cast<size_t>(m_iBlockSize));
    for(auto& bus : outputBuffer->GetBusConnections()) {
        bus.bridgeWriter->SetWriteSize(static_cast<unsigned>(m_iBlockSize));
    }

    const size_t totalIn = m_iBlockSize * numOutputs;
    inputReader->SetReadSize(static_cast<unsigned>(totalIn));
    for(auto& bus : inputReader->GetBusConnections()) {
        bus.bridgeReader->SetReadSize(static_cast<unsigned>(totalIn));
    }

    return true;
}
