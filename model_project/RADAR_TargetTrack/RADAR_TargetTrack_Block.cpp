#include "RADAR_TargetTrack_Block.h"

RADAR_TargetTrack_Block::RADAR_TargetTrack_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_TargetTrack_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    while(!m_GateStartQueue.empty()) m_GateStartQueue.pop();
    while(!m_RangeQueue.empty()) m_RangeQueue.pop();
    return true;
}

bool RADAR_TargetTrack_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_TargetTrack_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_radar = std::make_unique<RADAR_TargetTrack>();
    SetDefaultParameters();

    try { PRI_Or_WaveGate = std::stod(getParameter("PRI_Or_WaveGate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI_Or_WaveGate', using default value."); }
    try { TrackGate = std::stod(getParameter("TrackGate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TrackGate', using default value."); }
    try { InitGateStartTime = std::stoi(getParameter("InitGateStartTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitGateStartTime', using default value."); }
    try { SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("input", m_radar->input, static_cast<size_t>(PRINum), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("isTrack", m_radar->isTrack, 1, DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_radar->output, static_cast<size_t>(PRINum), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("GateStart", m_radar->GateStart, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Range", m_radar->Range, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void RADAR_TargetTrack_Block::SetDefaultParameters()
{
    PRI_Or_WaveGate = 10e-6;
    TrackGate = 10e-6;
    InitGateStartTime = 10e-6;
    SampleRate = 10e6;
}

bool RADAR_TargetTrack_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>("input");
    std::vector<bool> isTrackData = ReadInputData<bool>("isTrack");

    std::vector<std::complex<double>> outputData(PRINum);
    std::vector<int> GateStartData(1);
    std::vector<int> RangeData(1);

    if (isTrackData[0])
    {
        // 快时间维求距离量
        double maxValue = 0.0;
        int maxIndex = 0;
        for (int i = 0; i < PRINum; i++)
        {
            if (std::abs(inputData[i]) > maxValue)
            {
                maxValue = std::abs(inputData[i]);
                maxIndex = i;
            }
        }
        // 距离维输出
        maxIndex = fmod(maxIndex, PRINum);
        const double c = 3e8;
        RangeData[0] = c * ((maxIndex / SampleRate) + GateStartTime) / 2;

        // 门限控制时间根据目标与门限中心的偏移量变化，反馈给波门模型以使得目标尽可能落入门限中心，实现跟踪的目的
        GateStartTime += maxIndex / SampleRate - PRI_Or_WaveGate / 2;
        GateStartData[0] = GateStartTime;

        // 信号输出
        for (int i = 0; i < PRINum; i++)
        {
            outputData[i] = inputData[i];
        }
    }
    else
    {
        // 距离维输出
        RangeData[0] = 0;

        // 门限控制时间输出
        GateStartData[0] = InitGateStartTime;

        // 信号输出
        for (int i = 0; i < PRINum; i++)
        {
            outputData[i] = 0;
        }
    }

    WriteOutputData("output", outputData);
    WriteOutputData("Range", RangeData);
    WriteOutputData("GateStart", GateStartData);

    return true;
}

bool RADAR_TargetTrack_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<std::complex<double>>("input");
    auto isTrackData = ReadInputData<bool>("isTrack");

    if(inputData.empty() || isTrackData.empty()) return true;

    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    m_istrackBuffer.push_back(isTrackData[0]);

    if(m_inputBuffer.size() >= static_cast<size_t>(PRINum) && m_istrackBuffer.size() >= 1) {
        std::vector<std::complex<double>> outputData(PRINum);
        std::vector<int> GateStartData(1);
        std::vector<int> RangeData(1);

        if (m_istrackBuffer[0])
        {
            // 快时间维求距离量
            double maxValue = 0.0;
            int maxIndex = 0;
            for (int i = 0; i < PRINum; i++)
            {
                if (std::abs(m_inputBuffer[i]) > maxValue)
                {
                    maxValue = std::abs(m_inputBuffer[i]);
                    maxIndex = i;
                }
            }
            // 距离维输出
            maxIndex = fmod(maxIndex, PRINum);
            const double c = 3e8;
            RangeData[0] = c * ((maxIndex / SampleRate) + GateStartTime) / 2;

            // 门限控制时间根据目标与门限中心的偏移量变化，反馈给波门模型以使得目标尽可能落入门限中心，实现跟踪的目的
            GateStartTime += maxIndex / SampleRate - PRI_Or_WaveGate / 2;
            GateStartData[0] = GateStartTime;

            // 信号输出
            for (int i = 0; i < PRINum; i++)
            {
                outputData[i] = m_inputBuffer[i];
            }
        }
        else
        {
            // 距离维输出
            RangeData[0] = 0;

            // 门限控制时间输出
            GateStartData[0] = InitGateStartTime;

            // 信号输出
            for (int i = 0; i < PRINum; i++)
            {
                outputData[i] = 0;
            }
        }

        for(const auto& val : outputData) m_outputQueue.push(val);
        m_GateStartQueue.push(GateStartData[0]);
        m_RangeQueue.push(RangeData[0]);
        if(!m_GateStartQueue.empty() && !m_RangeQueue.empty()) {
            double GateStartValue = m_GateStartQueue.front();
            m_GateStartQueue.pop();
            WriteOutputData("GateStart", std::vector<double>{GateStartValue});
            m_lastGateStart = GateStartValue;

            double RangeValue = m_RangeQueue.front();
            m_RangeQueue.pop();
            WriteOutputData("Range", std::vector<double>{RangeValue});
            m_lastRange = RangeValue;
        }
    }
    if(!m_outputQueue.empty()) {
        std::complex<double> outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        WriteOutputData("output", std::vector<std::complex<double>>{outputValue});
        m_lastOutput = outputValue;
        m_inputBuffer.clear();
        qDebug() << "[RADAR_TargetTrack_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << outputValue.imag();
    }
    return true;
}

void RADAR_TargetTrack_Block::SetParameters()
{
    if(!m_radar) return;
    m_radar->PRI_Or_WaveGate = PRI_Or_WaveGate;
    m_radar->TrackGate = TrackGate;
    m_radar->InitGateStartTime = InitGateStartTime;
    m_radar->SampleRate = SampleRate;
}

bool RADAR_TargetTrack_Block::ModelSetup()
{
    bool bStatus = true;

    // 参数校验
    if (PRI_Or_WaveGate <= 0)
    {
        LOG_ERROR("PRI_Or_WaveGate must be > 0");
        bStatus = false;
    }
    if (TrackGate <= 0 || TrackGate > PRI_Or_WaveGate)
    {
        LOG_ERROR("TrackGate must be > 0 and <= PRI_Or_WaveGate");
        bStatus = false;
    }
    if (SampleRate <= 0)
    {
        LOG_ERROR("SampleRate must be > 0");
        bStatus = false;
    }

    // 设置端口速率
    PRINum = PRI_Or_WaveGate * SampleRate;

    // 门限控制时间初始化
    GateStartTime = InitGateStartTime;

    return bStatus;
}

