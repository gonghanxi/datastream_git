#include "PolarToCx_Block.h"

#include <complex>
#include <vector>

PolarToCx_Block::PolarToCx_Block(const std::string& name)
    : Block(name)
{
}

bool PolarToCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool PolarToCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_polarToCx = std::make_unique<PolarToCx>();

    AddInputPort("magnitude", m_polarToCx->magnitude, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("phase", m_polarToCx->phase, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_polarToCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

bool PolarToCx_Block::DataStreamRun()
{
    const std::string magPortName = GetInputPortName(0);
    const std::string phasePortName = GetInputPortName(1);
    const std::string outPortName = GetOutputPortName(0);

    auto magData = ReadInputData<double>(magPortName);
    auto phaseData = ReadInputData<double>(phasePortName);

    if (magData.empty() || phaseData.empty()) {
        return false;
    }

    const size_t count = std::min(magData.size(), phaseData.size());
    std::vector<std::complex<double>> outputData;
    outputData.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        outputData.push_back(std::polar(magData[i], phaseData[i]));
    }

    WriteOutputData(outPortName, outputData);
    return true;
}

bool PolarToCx_Block::TimeDrivenRun()
{
    const std::string magPortName = GetInputPortName(0);
    const std::string phasePortName = GetInputPortName(1);
    const std::string outPortName = GetOutputPortName(0);

    auto magData = ReadInputData<double>(magPortName);
    auto phaseData = ReadInputData<double>(phasePortName);

    if (magData.empty() || phaseData.empty()) {
        return true;
    }
    m_magBuffer.push_back(magData[0]);
    m_phaseBuffer.push_back(phaseData[0]);

    if(m_magBuffer.size() >= 1 && m_phaseBuffer.size() >= 1) {
        const size_t count = std::min(m_magBuffer.size(), m_phaseBuffer.size());
        std::vector<std::complex<double>> outputData;
        outputData.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            outputData.push_back(std::polar(m_magBuffer[i], m_phaseBuffer[i]));
        }
        m_outputQueue.push(outputData[0]);
        //执行写入
        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[PhaseShifter_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_magBuffer.clear();
            m_phaseBuffer.clear();
        }
    }
    return true;
}

bool PolarToCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}
