#include "InterleaveDeinterleaveCx_Block.h"

InterleaveDeinterleaveCx_Block::InterleaveDeinterleaveCx_Block(const std::string &name)
    :Block(name)
{

}
bool InterleaveDeinterleaveCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool InterleaveDeinterleaveCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool InterleaveDeinterleaveCx_Block::DataStreamRun()
{
    // 获取端口名称
    size_t outputRate = m_inter->output.GetRate();

    // 读取输入数据
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    std::vector<std::complex<double>> outputData(outputRate);

    for (size_t i = 0; i < static_cast<size_t>(Columns); ++i)
        {
            for (size_t j = 0; j < static_cast<size_t>(Rows); ++j)
            {
                // 计算输入和输出缓冲区中的索引
                size_t inputIndex = i + j * static_cast<size_t>(Columns);
                size_t outputIndex = j + i * static_cast<size_t>(Rows);

                outputData[outputIndex] = inputData[inputIndex];
            }
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool InterleaveDeinterleaveCx_Block::TimeDrivenRun()
{
    // 获取端口名称
    size_t outputRate = m_inter->output.GetRate();

    // 读取输入数据
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_blockSize)) {
        std::vector<std::complex<double>> outputData(outputRate);

        for (size_t i = 0; i < static_cast<size_t>(Columns); ++i)
        {
            for (size_t j = 0; j < static_cast<size_t>(Rows); ++j)
            {
                // 计算输入和输出缓冲区中的索引
                size_t inputIndex = i + j * static_cast<size_t>(Columns);
                size_t outputIndex = j + i * static_cast<size_t>(Rows);

                outputData[outputIndex] = m_inputBuffer[inputIndex];
            }
        }
        for(const auto& val : outputData) {
            m_outputQueue.push(val);
        }
        if (!m_outputQueue.empty())
        {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[InterleaveDeinterleaveCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }

    }
    return true;
}

bool InterleaveDeinterleaveCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_inter = std::make_unique<InterleaveDeinterleaveCx>();

    SetDefaultParameters();

    try { Rows = std::stoi(getParameter("Rows").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Rows', using default value."); }
    try { Columns = std::stoi(getParameter("Columns").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Columns', using default value."); }


    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("input", m_inter->input, static_cast<size_t>(m_blockSize), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_inter->output, static_cast<size_t>(m_blockSize), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

void InterleaveDeinterleaveCx_Block::SetParameters()
{
    if(!m_inter) return;
    m_inter->Rows = Rows;
    m_inter->Columns = Columns;
}

bool InterleaveDeinterleaveCx_Block::ModelSetup()
{
    bool bStatus = true;
    if (Rows < 1)
    {
        LOG_ERROR("Number of rows must be > 0 ");
        bStatus = false;
    }
    if (Columns < 1)
    {
        LOG_ERROR("Number of columns must be > 0 ");
        bStatus = false;
    }
    if (Rows > 0 && Columns > 0)
    {
        // 注意：与内置一致，不在这里做过多“修正”，只做正数判断
        const size_t N = static_cast<size_t>(Rows) * static_cast<size_t>(Columns);

        // SystemVue SetRate 通常是 unsigned
        m_blockSize = static_cast<unsigned>(N);

        m_inter->input.SetRate(m_blockSize);
        m_inter->output.SetRate(m_blockSize);
    }
    return bStatus;
}

void InterleaveDeinterleaveCx_Block::SetDefaultParameters()
{
    Rows = 8;
    Columns = 8;
}
