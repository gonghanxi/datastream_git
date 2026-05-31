#include "RectToCx_Block.h"

//template class RECTTOCX_API std::map<std::string, Parameter>;
//template class RECTTOCX_API std::map<int, PortMsg>;

RectToCx_Block::RectToCx_Block(const std::string &name)
    :Block(name)
{}

bool RectToCx_Block::Setup()
{
    Block::Setup();
    while(!m_OutputQueue.empty()) m_OutputQueue.pop();
    return true;
}

bool RectToCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RectToCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_rectoCx = std::make_unique<RectToCx>();

    AddInputPort("Real", m_rectoCx->Real, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Imag", m_rectoCx->Imag, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Cx", m_rectoCx->Cx, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

int RectToCx_Block::GetBatchSize() const
{
    // 动态调整批量大小（根据下游背压）
    float usage = GetDownstreamBufferUsage();

    if (usage > 80.0f) {
        return 1;  // 下游压力大，只处理1个
    } else if (usage > 60.0f) {
        return std::max(1, m_batchSize / 4);  // 中等压力，减少批量
    } else if (usage > 40.0f) {
        return std::max(1, m_batchSize / 2);
    }

    return m_batchSize;
}

int RectToCx_Block::RunBatch(int maxCount)
{

    if (GetInputPortCount() < 2) return 0;
    qDebug() << "RectToCx_Block::RunBatch - begin";
    std::string RealPortName = GetInputPortName(0);
    std::string ImagPortName = GetInputPortName(1);
    std::string outPortName = GetOutputPortName(0);

    BufferReader* RealReader = GetInputPort(RealPortName);
    BufferReader* ImagReader = GetInputPort(ImagPortName);

    // ========== 动态计算实际批量 ==========
    // 取两个端口中数据量较小的作为批量大小
    size_t RealAvailable = RealReader->GetAvailableDataCount();
    size_t ImagAvailable = ImagReader->GetAvailableDataCount();
    int batchSize = std::min((int)std::min(RealAvailable, ImagAvailable), maxCount);

    // 检查下游背压
    float usage = GetDownstreamBufferUsage();
    if (usage > 80.0f) {
        batchSize = std::min(batchSize, 1);
    } else if (usage > 60.0f) {
        batchSize = std::min(batchSize, std::max(1, GetBatchSize() / 4));
    }
    if (batchSize <= 0) return 0;

    std::vector<double> RealData(batchSize);
    std::vector<double> ImagData(batchSize);

    for (int i = 0; i < batchSize; i++) {

        //real 端已连接 且 imag 端未连接
        if(RealReader->IsConnected() && !ImagReader->IsConnected()) {
            auto RealData = ReadInputData<double>(RealPortName);
            RealData[i] = RealData[i];
            ImagData[i] = 0.0;
        }
        //real 端未连接 且 imag 端已连接
        else if(!RealReader->IsConnected() && ImagReader->IsConnected()) {
            auto ImagData = ReadInputData<double>(ImagPortName);
            RealData[i] = 0.0;
            ImagData[i] = ImagData[i];
        }
        //real 端已连接 且 imag 端已连接
        else if(RealReader->IsConnected() && ImagReader->IsConnected()) {
            auto realData = ReadInputData<double>(RealPortName);
            auto imagData = ReadInputData<double>(ImagPortName);
            RealData[i] = realData[i];
            ImagData[i] = imagData[i];
        }
    }

    // 批量处理（复用 Run 的逻辑：每对 I/Q 产生一个复数）
    std::vector<std::complex<double>> outputData;
    outputData.reserve(batchSize);

    for (int i = 0; i < batchSize; i++) {
        outputData.push_back(std::complex<double>(RealData[i], ImagData[i]));
    }

    // 批量写入
    if (!WriteOutputData(outPortName, outputData)) {
        return 0;
    }
    return batchSize;
}

bool RectToCx_Block::DataStreamRun()
{
    std::string RealPort = GetInputPortName(0);
    std::string ImagPort = GetInputPortName(1);
    std::string DComplexPort = GetOutputPortName(0);

    BufferReader* realPort = GetInputPort(RealPort);
    BufferReader* imagPort = GetInputPort(ImagPort);
    std::vector<std::complex<double>> DComplexData(1);

    //real 端已连接 且 imag 端未连接
    if(realPort->IsConnected() && !imagPort->IsConnected()) {
        auto RealData = ReadInputData<double>(RealPort);
        DComplexData[0] = std::complex<double>(RealData[0],0.0);
    }
    //real 端未连接 且 imag 端已连接
    else if(!realPort->IsConnected() && imagPort->IsConnected()) {
        auto ImagData = ReadInputData<double>(ImagPort);
        DComplexData[0] = std::complex<double>(0.0,ImagData[0]);
    }
    //real 端已连接 且 imag 端已连接
    else if(realPort->IsConnected() && imagPort->IsConnected()) {
        auto RealData = ReadInputData<double>(RealPort);
        auto ImagData = ReadInputData<double>(ImagPort);
        DComplexData[0] = std::complex<double>(RealData[0],ImagData[0]);
    }
    WriteOutputData(DComplexPort, DComplexData);
    return true;
}

bool RectToCx_Block::TimeDrivenRun()
{
    std::string RealPort = GetInputPortName(0);
    std::string ImagPort = GetInputPortName(1);
    std::string DComplexPort = GetOutputPortName(0);

    BufferReader* realPort = GetInputPort(RealPort);
    BufferReader* imagPort = GetInputPort(ImagPort);
    std::vector<std::complex<double>> DComplexData(1);

    //real 端已连接 且 imag 端未连接
    if(realPort->IsConnected() && !imagPort->IsConnected()) {
        auto RealData = ReadInputData<double>(RealPort);
        if(RealData.empty()) return true;

        m_realBuffer.push_back(RealData[0]);
        if(m_realBuffer.size() >=1 ) {
            DComplexData[0] = std::complex<double>(RealData[0],0.0);
            m_OutputQueue.push(DComplexData[0]);
            if(!m_OutputQueue.empty()) {
                std::complex<double> outputValue = m_OutputQueue.front();
                m_OutputQueue.pop();
                m_outputCount++;

                WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});

                m_lastOutput = outputValue;

                qDebug() << "[RectToCx_Block] 分发输出:" << m_outputCount
                         << " value:" << outputValue.real() << "," << outputValue.imag();
                m_realBuffer.clear();
                m_imagBuffer.clear();
            }
        }

    }
    //real 端未连接 且 imag 端已连接
    else if(!realPort->IsConnected() && imagPort->IsConnected()) {
        auto ImagData = ReadInputData<double>(ImagPort);
        if(ImagData.empty()) return true;

        m_imagBuffer.push_back(ImagData[0]);
        if(m_imagBuffer.size() >=1 ) {
            DComplexData[0] = std::complex<double>(0.0,ImagData[0]);
            m_OutputQueue.push(DComplexData[0]);
            if(!m_OutputQueue.empty()) {
                std::complex<double> outputValue = m_OutputQueue.front();
                m_OutputQueue.pop();
                m_outputCount++;

                WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});

                m_lastOutput = outputValue;

                qDebug() << "[RectToCx_Block] 分发输出:" << m_outputCount
                         << " value:" << outputValue.real() << "," << outputValue.imag();
                m_realBuffer.clear();
                m_imagBuffer.clear();
            }
        }
    }
    //real 端已连接 且 imag 端已连接
    else if(realPort->IsConnected() && imagPort->IsConnected()) {
        auto RealData = ReadInputData<double>(RealPort);
        auto ImagData = ReadInputData<double>(ImagPort);
        if(RealData.empty() || ImagData.empty()) return true;

        m_realBuffer.push_back(RealData[0]);
        m_imagBuffer.push_back(ImagData[0]);
        if(m_imagBuffer.size() >=1 ) {
            DComplexData[0] = std::complex<double>(RealData[0],ImagData[0]);
            m_OutputQueue.push(DComplexData[0]);
            if(!m_OutputQueue.empty()) {
                std::complex<double> outputValue = m_OutputQueue.front();
                m_OutputQueue.pop();
                m_outputCount++;

                WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});

                m_lastOutput = outputValue;

                qDebug() << "[RectToCx_Block] 分发输出:" << m_outputCount
                         << " value:" << outputValue.real() << "," << outputValue.imag();
                m_realBuffer.clear();
                m_imagBuffer.clear();
            }
        }
    }
    return true;
}
