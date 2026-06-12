#include "RADAR_WaveGate_Block.h"

RADAR_WaveGate_Block::RADAR_WaveGate_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_WaveGate_Block::Setup()
{
    Block::Setup();
    while(!m_OutputQueue.empty()) m_OutputQueue.pop();
    return true;
}

void RADAR_WaveGate_Block::SetDefaultParameters()
{
    PRF = 10e3;
    StartTime = 0;
    GateTime = 20e-6;
    SampleRate = 10e6;
}

bool RADAR_WaveGate_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string gatePort = GetInputPortName(1);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    std::vector<std::complex<double>> outputData(m_radar->output.GetRate());

    if (GetInputPort(gatePort)->IsConnected())
    {
        auto gateData = ReadInputData<double>(gatePort);
        StartTime = gateData[0];
    }

    int StartN	= StartTime * SampleRate;
    int GateN = GateTime * SampleRate;
    int StopN = StartN + GateN;

    // 每个PRI都设一个波门
    for (int i = 0; i < StopN; i++)
    {
        outputData[i] = inputData[i + StartN];
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool RADAR_WaveGate_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string gatePort = GetInputPortName(1);
    size_t inputRate = GetInputPort(inputPort)->GetReadSize();
    size_t outputRate = GetOutputPort(GetOutputPortName(0))->GetWriteSize();

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if(inputData.empty()) return true;

    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if (GetInputPort(gatePort)->IsConnected())
    {
        auto gateData = ReadInputData<double>(gatePort);
        StartTime = gateData[0];
        m_GCBuffer.push_back(gateData[0]);
    }

    bool CanprocessData = false;
    if(GetInputPort(gatePort)->IsConnected()) {
        if(m_inputBuffer.size() >= inputRate && m_GCBuffer.size() >= 1) {
            CanprocessData = true;
        }
    }
    else {
        if(m_inputBuffer.size() >= inputRate) {
            CanprocessData = true;
        }
    }
    if(CanprocessData) {
        std::vector<std::complex<double>> outputData(outputRate);
        int StartN	= StartTime * SampleRate;
        int GateN = GateTime * SampleRate;
        int StopN = StartN + GateN;

        // 每个PRI都设一个波门
        for (int i = 0; i < StopN; i++)
        {
            outputData[i] = m_inputBuffer[i + StartN];
        }
        for(const auto& val : outputData) m_OutputQueue.push(val);
        if (!m_OutputQueue.empty()) {
            std::complex<double> outputValue = m_OutputQueue.front();
            m_OutputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});

            m_lastOutput = outputValue;

            qDebug() << "[RADAR_WaveGate_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
        }
    }
    return true;
}

void RADAR_WaveGate_Block::SetParameters()
{
    if (!m_radar) return;


    m_radar->PRF = PRF;
    m_radar->StartTime = StartTime;
    m_radar->GateTime = GateTime;
    m_radar->SampleRate = SampleRate;
}

bool RADAR_WaveGate_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radar = std::make_unique<RADAR_WaveGate>();

    SetDefaultParameters();

    try { PRF = std::stod(getParameter("PRF").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRF', using default value."); }
    try { StartTime = std::stod(getParameter("StartTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartTime', using default value."); }
    try { GateTime = std::stod(getParameter("GateTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'GateTime', using default value."); }
    try { SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    SetParameters();

    double PRI = 1.0 / PRF;

    int inputRate = PRI * SampleRate;
    int outputRate = GateTime * SampleRate;

    if (inputRate > 0 && outputRate > 0)
    {
        m_radar->input.SetRate(inputRate);
        m_radar->output.SetRate(outputRate);
    }
    else
    {
        LOG_ERROR("Port rate must be greater than 0.");
        return false;
    }
    AddInputPort("input", m_radar->input, static_cast<size_t>(inputRate), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("GateStartCtrl", m_radar->GateStartCtrl, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_radar->output, static_cast<size_t>(outputRate), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

bool RADAR_WaveGate_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}
