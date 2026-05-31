#include "AsyncCommutatorCx_Block.h"

#include <algorithm>
#include <vector>

AsyncCommutatorCx_Block::AsyncCommutatorCx_Block(const std::string& name)
    : Block(name)
    , m_blockSizes(1, 1)
{
    m_blockSizes(0) = 1;
}

void AsyncCommutatorCx_Block::SetDefaultParamters()
{
    m_blockSizes.Resize(1, 1);
    m_blockSizes(0) = 1;
}

void AsyncCommutatorCx_Block::SetParameters()
{
    if (!m_asyncCommutatorCx) {
        return;
    }

    m_asyncCommutatorCx->BlockSizes = m_blockSizes;
}

bool AsyncCommutatorCx_Block::Setup()
{
    Block::Setup();
    if(!ModelSetup()) return false;
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

bool AsyncCommutatorCx_Block::Run()
{
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool AsyncCommutatorCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_asyncCommutatorCx = std::make_unique<AsyncCommutatorCx>();

    AddInputPort("input", m_asyncCommutatorCx->input, 1, Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_asyncCommutatorCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_blockSizes = DataTypesAndParsers::ParseStringToMatrixInt(getParameter("BlockSizes").Value); } catch (...) { }

    SetParameters();

    return true;
}

bool AsyncCommutatorCx_Block::ModelSetup()
{
    auto* inputPort = GetInputPort(GetInputPortName(0));
    auto* outputPort = GetOutputPort(GetOutputPortName(0));

    const size_t numInputs = inputPort ? inputPort->GetBusConnectionCount() : 0U;
    const size_t numBlockSizes = static_cast<size_t>(m_blockSizes.NumElements());

    if (numInputs > 0 && numBlockSizes != numInputs) {
        LOG_ERROR("AsyncCommutator: Size of BlockSizes array must equal number of inputs.");
        return false;
    }

    size_t numOutputSamples = 0;

    for (size_t i = 0; i < numBlockSizes; ++i) {
        const int Bi = m_blockSizes(i);
        if (Bi > 0) {
            numOutputSamples += static_cast<size_t>(Bi);
            m_maxBlock = std::max(m_maxBlock, static_cast<size_t>(Bi));
        } else if (Bi < 0) {
            LOG_ERROR("AsyncCommutator: Elements of BlockSizes must be >= 0.");
            return false;
        }
    }

    if (numOutputSamples == 0) {
        LOG_ERROR("AsyncCommutator: At least one BlockSizes element must be > 0.");
        return false;
    }

    if (inputPort) {
        for(auto& connection : inputPort->GetBusConnections()) {
            connection.bridgeReader->SetReadSize(m_maxBlock > 0 ? m_maxBlock : 1U);
        }
    }
    if (outputPort) {
        outputPort->SetWriteSize(numOutputSamples);
    }
    return true;
}

bool AsyncCommutatorCx_Block::DataStreamRun()
{
    const std::string inputPortName = GetInputPortName(0);
    const std::string outputPortName = GetOutputPortName(0);

    const size_t numBlockSizes = static_cast<size_t>(m_blockSizes.NumElements());

    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    std::vector<std::complex<double>> outputData;
    size_t totalExpected = 0;
    for (size_t i = 0; i < numBlockSizes; ++i) {
        const int Bi = m_blockSizes(i);
        if (Bi > 0) {
            totalExpected += static_cast<size_t>(Bi);
        }
    }
    outputData.reserve(std::min(totalExpected, inputData.size()));

    size_t offset = 0;
    for (size_t i = 0; i < numBlockSizes && offset < inputData.size(); ++i) {
        const int Bi = m_blockSizes(i);
        if (Bi <= 0) {
            continue;
        }
        const size_t count = std::min(static_cast<size_t>(Bi), inputData.size() - offset);
        outputData.insert(outputData.end(), inputData.begin() + static_cast<long long>(offset), inputData.begin() + static_cast<long long>(offset + count));
        offset += count;
    }

    WriteOutputData(outputPortName, outputData);
    return true;
}

bool AsyncCommutatorCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();

    // 步骤1：从每个子端口读取新数据，追加到对应的累积缓冲区
    for (const auto& bridge_reader : bridge_readers) {
        std::vector<std::complex<double>> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if (!inputData.empty()) {
            m_inputBuffer[bridge_reader.bridgeReader].insert(
                m_inputBuffer[bridge_reader.bridgeReader].end(),
                inputData.begin(), inputData.end());
        }
    }

    // 步骤2：检查是否每个子端口的累积数据量都 >= 对应的 BlockSize
    bool CanProcessData = true;
    const int numInputs = m_blockSizes.NumElements();
    if (numInputs != static_cast<int>(bridge_readers.size())) {
        // 配置错误：BlockSizes 数组大小与子端口数量不匹配
        CanProcessData = false;
    } else {
        for (int i = 0; i < numInputs; ++i) {
            auto* reader = bridge_readers[i].bridgeReader;
            auto it = m_inputBuffer.find(reader);
            if (it == m_inputBuffer.end() ||
                it->second.size() < static_cast<size_t>(m_blockSizes(i))) {
                CanProcessData = false;
                break;
            }
        }
    }

    // 步骤3：当所有子端口都满足条件时，拼接输出向量并填充输出队列
    if (CanProcessData) {
        // 计算本次处理需要输出的总样本数
        size_t totalSamples = 0;
        for (int i = 0; i < numInputs; ++i) {
            totalSamples += static_cast<size_t>(m_blockSizes(i));
        }

        // 按照 BlockSizes 顺序从每个子端口缓冲区取出数据，组成一个输出块
        std::vector<double> outputBlock;
        outputBlock.reserve(totalSamples);
        for (int i = 0; i < numInputs; ++i) {
            auto* reader = bridge_readers[i].bridgeReader;
            int blockSize = m_blockSizes(i);
            auto& buf = m_inputBuffer[reader];
            // 取出前 blockSize 个元素
            outputBlock.insert(outputBlock.end(), buf.begin(), buf.begin() + blockSize);
            // 从缓冲区中删除已取出的元素
            buf.erase(buf.begin(), buf.begin() + blockSize);
        }


        for (std::complex<double> val : outputBlock) {
            m_outputQueue.push(val);
        }

        qDebug() << "[AsyncCommutator] " << outputBlock.size()
                 << ":" << m_outputQueue.size();
    }


    if (!m_outputQueue.empty()) {
        std::complex<double> outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[AsyncCommutator] :" << m_outputCount
                 << " value:" << outputValue.real() << "," << outputValue.imag();
    } else {

        // WriteOutputData(outputPort, std::vector<double>{m_lastOutput});
        qDebug() << "[AsyncCommutator]";
    }

    return true;
}
