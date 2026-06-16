#include "RADAR_TargetTrack_M_Block.h"

RADAR_TargetTrack_M_Block::RADAR_TargetTrack_M_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_TargetTrack_M_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    while(!m_GateStartQueue.empty()) m_GateStartQueue.pop();
    while(!m_RangeQueue.empty()) m_RangeQueue.pop();
    return true;
}

bool RADAR_TargetTrack_M_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_TargetTrack_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_radar = std::make_unique<RADAR_TargetTrack_M>();
    SetDefaultParameters();

    try { PRI_Or_WaveGate = std::stod(getParameter("PRI_Or_WaveGate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI_Or_WaveGate', using default value."); }
    try { TrackGate = std::stod(getParameter("TrackGate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TrackGate', using default value."); }
    try { InitGateStartTime = std::stoi(getParameter("InitGateStartTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitGateStartTime', using default value."); }
    try { SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("input", m_radar->input, static_cast<size_t>(PRINum), DataType::MATRIX_DCOMPLEX);
    AddInputPort("isTrack", m_radar->isTrack, 1, DataType::MATRIX_BOOL);
    AddOutputPort("output", m_radar->output, static_cast<size_t>(PRINum), DataType::MATRIX_DCOMPLEX);
    AddOutputPort("GateStart", m_radar->GateStart, 1, DataType::MATRIX_DOUBLE);
    AddOutputPort("Range", m_radar->Range, 1, DataType::MATRIX_DOUBLE);
    return true;
}

void RADAR_TargetTrack_M_Block::SetDefaultParameters()
{
    PRI_Or_WaveGate = 10e-6;
    TrackGate = 10e-6;
    InitGateStartTime = 10e-6;
    SampleRate = 10e6;
}

bool RADAR_TargetTrack_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<DComplexMatrix>("input");
    auto isTrackData = ReadInputData<BoolMatrix>("isTrack");

    if(inputData.empty() || isTrackData.empty()) return true;

    numRows = static_cast<int>(inputData[0].NumRows());
    numCols = static_cast<int>(inputData[0].NumColumns());

    std::vector<DComplexMatrix> outputData(PRINum);
    for(int i = 0; i < PRINum; i++) outputData[i] = DComplexMatrix(numRows, numCols);

    std::vector<DoubleMatrix> GateStartData(1);
    std::vector<DoubleMatrix> RangeData(1);
    GateStartData[0] = DoubleMatrix(numRows, numCols);
    RangeData[0] = DoubleMatrix(numRows, numCols);

    for (int m = 0; m < numRows; m++)
    {
        for (int n = 0; n < numCols; n++)
        {
            if (isTrackData[0](m, n))
            {
                // 快时间维求距离量
                double maxValue = 0.0;
                int maxIndex = 0;
                for (int i = 0; i < PRINum; i++)
                {
                    if (std::abs(inputData[i](m, n)) > maxValue)
                    {
                        maxValue = std::abs(inputData[i](m, n));
                        maxIndex = i;
                    }
                }
                // 距离维输出
                maxIndex = fmod(maxIndex, PRINum);
                const double c = 3e8;
                RangeData[0](m, n) = c * ((maxIndex / SampleRate) + GateStartTime) / 2;

                // 门限控制时间根据目标与门限中心的偏移量变化，反馈给波门模型以使得目标尽可能落入门限中心，实现跟踪的目的
                GateStartTime += maxIndex / SampleRate - PRI_Or_WaveGate / 2;
                GateStartData[0](m, n) = GateStartTime;

                // 信号输出
                for (int i = 0; i < PRINum; i++)
                {
                    outputData[i](m, n) = inputData[i](m, n);
                }
            }
            else
            {
                // 距离维输出
                RangeData[0](m, n) = 0;

                // 门限控制时间输出
                GateStartData[0](m, n) = InitGateStartTime;

                // 信号输出
                for (int i = 0; i < PRINum; i++)
                {
                    outputData[i](m, n) = 0;
                }
            }
        }
    }

    WriteOutputData("output", outputData);
    WriteOutputData("Range", RangeData);
    WriteOutputData("GateStart", GateStartData);

    return true;
}

bool RADAR_TargetTrack_M_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<DComplexMatrix>("input");
    auto isTrackData = ReadInputData<BoolMatrix>("isTrack");

    if(inputData.empty() || isTrackData.empty()) return true;

    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    m_istrackBuffer.push_back(isTrackData[0]);

    if(m_inputBuffer.size() >= static_cast<size_t>(PRINum) && m_istrackBuffer.size() >= 1) {
        numRows = static_cast<int>(m_inputBuffer[0].NumRows());
        numCols = static_cast<int>(m_inputBuffer[0].NumColumns());

        std::vector<DComplexMatrix> outputData(PRINum);
        for(int i = 0; i < PRINum; i++) outputData[i] = DComplexMatrix(numRows, numCols);

        std::vector<DoubleMatrix> GateStartData(1);
        std::vector<DoubleMatrix> RangeData(1);
        GateStartData[0] = DoubleMatrix(numRows, numCols);
        RangeData[0] = DoubleMatrix(numRows, numCols);

        for (int m = 0; m < numRows; m++)
        {
            for (int n = 0; n < numCols; n++)
            {
                if (m_istrackBuffer[0](m, n))
                {
                    // 快时间维求距离量
                    double maxValue = 0.0;
                    int maxIndex = 0;
                    for (int i = 0; i < PRINum; i++)
                    {
                        if (std::abs(m_inputBuffer[i](m, n)) > maxValue)
                        {
                            maxValue = std::abs(m_inputBuffer[i](m, n));
                            maxIndex = i;
                        }
                    }
                    // 距离维输出
                    maxIndex = fmod(maxIndex, PRINum);
                    const double c = 3e8;
                    RangeData[0](m, n) = c * ((maxIndex / SampleRate) + GateStartTime) / 2;

                    // 门限控制时间根据目标与门限中心的偏移量变化
                    GateStartTime += maxIndex / SampleRate - PRI_Or_WaveGate / 2;
                    GateStartData[0](m, n) = GateStartTime;

                    // 信号输出
                    for (int i = 0; i < PRINum; i++)
                    {
                        outputData[i](m, n) = m_inputBuffer[i](m, n);
                    }
                }
                else
                {
                    // 距离维输出
                    RangeData[0](m, n) = 0;

                    // 门限控制时间输出
                    GateStartData[0](m, n) = InitGateStartTime;

                    // 信号输出
                    for (int i = 0; i < PRINum; i++)
                    {
                        outputData[i](m, n) = 0;
                    }
                }
            }
        }

        for(const auto& val : outputData) m_outputQueue.push(val);
        m_GateStartQueue.push(GateStartData[0]);
        m_RangeQueue.push(RangeData[0]);
    }

    // 分发GateStart和Range（每次处理完成后分发一次）
    if(!m_GateStartQueue.empty() && !m_RangeQueue.empty()) {
        auto GateStartValue = m_GateStartQueue.front();
        m_GateStartQueue.pop();
        WriteOutputData("GateStart", std::vector<DoubleMatrix>{GateStartValue});
        m_lastGateStart = GateStartValue;

        auto RangeValue = m_RangeQueue.front();
        m_RangeQueue.pop();
        WriteOutputData("Range", std::vector<DoubleMatrix>{RangeValue});
        m_lastRange = RangeValue;
    }

    // 分发output（逐个矩阵分发）
    if(!m_outputQueue.empty()) {
        auto outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        WriteOutputData("output", std::vector<DComplexMatrix>{outputValue});
        m_lastOutput = outputValue;
        qDebug() << "[RADAR_TargetTrack_M_Block] 分发输出:" << m_outputCount;
        m_inputBuffer.clear();
    }
    return true;
}

void RADAR_TargetTrack_M_Block::SetParameters()
{
    if(!m_radar) return;
    m_radar->PRI_Or_WaveGate = PRI_Or_WaveGate;
    m_radar->TrackGate = TrackGate;
    m_radar->InitGateStartTime = InitGateStartTime;
    m_radar->SampleRate = SampleRate;
}

bool RADAR_TargetTrack_M_Block::ModelSetup()
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

    // 矩阵维度在Initialize时未知，将在首次运行时从输入数据获取
    numRows = 0;
    numCols = 0;

    return bStatus;
}
