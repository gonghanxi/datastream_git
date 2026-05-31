#include "SubCx_Block.h"

SubCx_Block::SubCx_Block(const std::string& name)
    : Block(name)
{
}

void SubCx_Block::SetDefaultParamters()
{
}

bool SubCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool SubCx_Block::DataStreamRun()
{
    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto posData = ReadInputData<std::complex<double>>(posPort);
    if (posData.empty()) {
        return false;
    }

    auto negData = ReadInputData<std::complex<double>>(negPort);

    std::complex<double> acc = posData[0];
    for (size_t i = 0; i < negData.size(); ++i) {
        acc -= negData[i];
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(acc);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool SubCx_Block::TimeDrivenRun()
{
    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(negPort);
    auto bridge_readers = master_reader->GetBusConnections();

    for(const auto& bridge_reader : bridge_readers) {
        std::vector<std::complex<double>> negData;
        bridge_reader.bridgeReader->ReadData(negData);
        if(negData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < negData.size();i++) {
            m_negBuffer[bridge_reader.bridgeReader].push_back(negData[i]);
        }
    }

    auto posData = ReadInputData<std::complex<double>>(posPort);
    if (posData.empty()) {
        return true;
    }
    m_posBuffer.push_back(posData[0]);

    bool CanProcessData = true;
    for(auto it = m_negBuffer.begin(); it != m_negBuffer.end(); ++it) {
        if(it->second.size() >= 1) {
            CanProcessData = false;
            break;
        }
    }
    if(m_posBuffer.size() < 1) {
        CanProcessData = false;
    }
    if(CanProcessData) {
        std::complex<double> acc = m_posBuffer[0];
        for(auto it = m_negBuffer.begin(); it != m_negBuffer.end(); ++it) {
            acc -= it->second[0];
        }
        std::vector<std::complex<double>> outputData;
        outputData.push_back(acc);
        m_outputQueue.push(outputData[0]);

        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[SubCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_posBuffer.clear();
            m_negBuffer.clear();
        }
    }
    return true;
}

bool SubCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool SubCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_subCx = std::make_unique<SubCx>();

    AddInputPort("pos", m_subCx->pos, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("neg", m_subCx->neg, 1, Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_subCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    return true;
}
