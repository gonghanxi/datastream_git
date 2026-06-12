#include "AsyncDistributorCx_Block.h"

AsyncDistributorCx_Block::AsyncDistributorCx_Block(const std::string &name)
    :Block(name)
{

}
bool AsyncDistributorCx_Block::Setup()
{
    Block::Setup();
    if(!ModelsSetup()) return false;
    return true;
}

bool AsyncDistributorCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool AsyncDistributorCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_AsyncDistributor = std::make_unique<AsyncDistributorCx>();

    SetDefaultParameters();

    try { m_BlockSizes = ParseStringToMatrix<int>(getParameter("BlockSizes").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BlockSizes', using default value."); }

    SetParameters();

    AddInputPort("input", m_AsyncDistributor->input, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_AsyncDistributor->output, 1, DataType::DCOMPLEX_BUS);
    return true;
}

void AsyncDistributorCx_Block::SetParameters()
{
    if(!m_AsyncDistributor) return;
    m_AsyncDistributor->BlockSizes = m_BlockSizes;
}

void AsyncDistributorCx_Block::SetDefaultParameters()
{
    m_BlockSizes.Resize(1,1);
    m_BlockSizes(0,0) = 1;
}

bool AsyncDistributorCx_Block::ModelsSetup()
{
    Buffer* outputBuffer = GetOutputPort(GetOutputPortName(0));
    BufferReader* inputReader = GetInputPort(GetInputPortName(0));
    const size_t numOutputs = outputBuffer->GetBusConnectionCount();
    const size_t numBlockVals = m_BlockSizes.NumElements();

    m_blockSizes.clear();

    if (numOutputs == 0)
        return true;

    if (numBlockVals == 0)
    {
        LOG_ERROR("AsyncDistributor: BlockSizes must contain at least one element.");
        return false;
    }

    m_blockSizes.resize(numOutputs);

    if (numBlockVals == 1)
    {
        int B = m_BlockSizes(0);
        if (B <= 0)
        {
            LOG_ERROR("AsyncDistributor: elements of BlockSizes array must all be > 0.");
            return false;
        }

        for (size_t i = 0; i < numOutputs; ++i)
            m_blockSizes[i] = B;

        std::stringstream ss;
        ss << "AsyncDistributor: BlockSizes has 1 element, applying this value to all "
            << numOutputs << " output(s).";
        LOG_INFO(ss.str().c_str());
    }
    else if (numBlockVals >= numOutputs)
    {
        for (size_t i = 0; i < numOutputs; ++i)
        {
            int B = m_BlockSizes(i);
            if (B <= 0)
            {
                LOG_ERROR("AsyncDistributor: elements of BlockSizes array must all be > 0.");
                return false;
            }
            m_blockSizes[i] = B;
        }

        if (numBlockVals > numOutputs)
        {
            std::stringstream ss;
            ss << "AsyncDistributor: BlockSizes has "
                << numBlockVals << " element(s), but only "
                << numOutputs << " output(s) are connected. "
                << "Ignoring the extra elements.";
            LOG_INFO(ss.str().c_str());
        }
    }
    else // 1 < numBlockVals < numOutputs
    {
        for (size_t i = 0; i < numBlockVals; ++i)
        {
            int B = m_BlockSizes(i);
            if (B <= 0)
            {
                LOG_ERROR("AsyncDistributor: elements of BlockSizes array must all be > 0.");
                return false;
            }
            m_blockSizes[i] = B;
        }

        int lastB = m_BlockSizes(numBlockVals - 1);
        for (size_t i = numBlockVals; i < numOutputs; ++i)
            m_blockSizes[i] = lastB;

        std::stringstream ss;
        ss << "AsyncDistributor: BlockSizes has " << numBlockVals
            << " element(s), but " << numOutputs
            << " output(s) are connected. Reusing the last element for remaining outputs.";
        LOG_INFO(ss.str().c_str());
    }

    size_t numInputData = 0;
    for (size_t i = 0; i < numOutputs; ++i)
    {
        int Bi = m_blockSizes[i];
        if (Bi <= 0)
        {
            LOG_ERROR("AsyncDistributor: expanded BlockSizes element is not > 0.");
            return false;
        }

        numInputData += static_cast<size_t>(Bi);
        outputBuffer->SetWriteSize(static_cast<size_t>(Bi));
        for(auto& bus : outputBuffer->GetBusConnections()) {
            bus.bridgeWriter->SetWriteSize(static_cast<unsigned>(Bi));
        }
    }

    if (numInputData == 0)
    {
        LOG_ERROR("AsyncDistributor: total number of input samples per run must be > 0.");
        return false;
    }

    inputReader->SetReadSize(static_cast<unsigned>(numInputData));
    m_channelQueues.resize(numOutputs);
    return true;
}

bool AsyncDistributorCx_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    // 读取所有输入数据
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPort);

    // 获取输出 Buffer 和总线连接数
    Buffer* outputBuffer = GetOutputPort(outputPort);

    const size_t numOutputs = outputBuffer->GetBusConnectionCount();
    if (numOutputs == 0 || m_blockSizes.empty()) {
        return true;
    }

    // 检查输入数据大小是否匹配
    size_t totalRequiredSize = 0;
    for (size_t i = 0; i < numOutputs; ++i) {
        totalRequiredSize += static_cast<size_t>(m_blockSizes[i]);
    }

    if (inputData.size() < totalRequiredSize) {
        LOG_ERROR("AsyncDistributor: Input data size (%zu) is less than required total block size (%zu)",
                  inputData.size(), totalRequiredSize);
        return false;
    }

    size_t k = 0;  // 当前在 inputData 中的读取位置

    // 为每个输出通道准备数据并写入
    for (size_t i = 0; i < numOutputs; ++i) {
        const int Bi = m_blockSizes[i];
        if (Bi <= 0) continue;

        // 提取当前通道的数据块
        std::vector<std::complex<double>> channelData;
        channelData.reserve(static_cast<size_t>(Bi));

        for (int j = 0; j < Bi; ++j) {
            if (k + j < inputData.size()) {
                channelData.push_back(inputData[k + j]);
            }
        }
        outputBuffer->WriteDataToChannel(static_cast<int>(i), channelData);

        k += static_cast<size_t>(Bi);
    }

    return true;
}

bool AsyncDistributorCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    // 获取输入端口（用于读取配置的 ReadSize）
    BufferReader* inputReader = GetInputPort(inputPort);

    // 获取输出 Buffer 对象
    Buffer* outputBuffer = GetOutputPort(outputPort);

    // 1. 读取本次输入数据并累积
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPort);
    if (!inputData.empty()) {
        m_inputBuffer.insert(m_inputBuffer.end(), inputData.begin(), inputData.end());
    }

    // 2. 检查是否累积足够的数据进行一次完整分发
    size_t requiredTotal = inputReader->GetReadSize();  // 在 ModelsSetup 中设置的值
    if (m_inputBuffer.size() >= requiredTotal && requiredTotal > 0) {
        const size_t numOutputs = outputBuffer->GetBusConnectionCount();
        if (numOutputs > 0 && m_blockSizes.size() >= numOutputs) {
            size_t offset = 0;
            // 按顺序为每个输出通道填充队列
            for (size_t i = 0; i < numOutputs; ++i) {
                int Bi = m_blockSizes[i];
                if (Bi <= 0) continue;
                size_t blockSize = static_cast<size_t>(Bi);
                // 确保队列容量足够（如果之前未调整大小，可在此检查）
                if (i >= m_channelQueues.size()) {
                    m_channelQueues.resize(i + 1);
                }
                // 将本通道的数据块逐个推入队列
                for (size_t j = 0; j < blockSize; ++j) {
                    if (offset + j < m_inputBuffer.size()) {
                        m_channelQueues[i].push(m_inputBuffer[offset + j]);
                    }
                }
                offset += blockSize;
            }
            // 清除已处理的输入数据
            m_inputBuffer.erase(m_inputBuffer.begin(), m_inputBuffer.begin() + offset);
        }
    }

    // 3. 从每个输出通道的队列中弹出一个样本并输出
    bool anyOutput = false;
    for (size_t i = 0; i < m_channelQueues.size(); ++i) {
        if (!m_channelQueues[i].empty()) {
            std::complex<double> sample = m_channelQueues[i].front();
            m_channelQueues[i].pop();
            // 写入单个样本到对应通道
            outputBuffer->WriteDataToChannel(static_cast<int>(i), std::vector<std::complex<double>>{sample});
            anyOutput = true;
            m_lastOutput = sample;   // 记录最后输出的值（可选）
        }
        // 如果队列为空，可以不写入任何数据（下游会等待）
    }
    return true;
}

