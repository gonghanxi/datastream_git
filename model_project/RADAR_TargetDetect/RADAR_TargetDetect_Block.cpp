#include "RADAR_TargetDetect_Block.h"
namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}
RADAR_TargetDetect_Block::RADAR_TargetDetect_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_TargetDetect_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    while(!m_IsDetectQueue.empty()) m_IsDetectQueue.pop();
    while(!m_RangeBinIndexQueue.empty()) m_RangeBinIndexQueue.pop();
    while(!m_FreqBinIndexQueue.empty()) m_FreqBinIndexQueue.pop();
    return true;
}

bool RADAR_TargetDetect_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_TargetDetect_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_radar = std::make_unique<RADAR_TargetDetect>();
    SetDefaultParameters();

    try {
        DetectType = ConvertStringToSelectedDetectType(getParameter("DetectType").Value);

        PRI_Or_WaveGate = std::stod(getParameter("PRI_Or_WaveGate").Value);
        FalseAlarmProbability = std::stod(getParameter("FalseAlarmProbability").Value);
        ReferenceCell = std::stoi(getParameter("ReferenceCell").Value);
        GuardCell = std::stoi(getParameter("GuardCell").Value);
        FreqChannelNum = std::stoi(getParameter("FreqChannelNum").Value);
        SampleRate = std::stod(getParameter("SampleRate").Value);

    } catch (...) {

    }

    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("input", m_radar->input, static_cast<size_t>(CellSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("IsDetect", m_radar->IsDetect, 1, DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_radar->output, static_cast<size_t>(CellSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("RangeBinIndex", m_radar->RangeBinIndex, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("FreqBinIndex", m_radar->FreqBinIndex, 1, DataType::CIRCULAR_BUFFER_INT);
    return true;
}

void RADAR_TargetDetect_Block::SetDefaultParameters()
{
    PRI_Or_WaveGate = 10e-3;
    DetectType = RADAR_TargetDetect::DetectRange;
    FalseAlarmProbability = 1e-6;
    ReferenceCell = 32;
    GuardCell = 4;
    FreqChannelNum = 32;
    SampleRate = 10e6;
}

RADAR_TargetDetect::SelectedDetectType RADAR_TargetDetect_Block::ConvertStringToSelectedDetectType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "detectrange" || lower == "0") return RADAR_TargetDetect::DetectRange;
    if(lower == "detect2d" || lower == "1") return RADAR_TargetDetect::Detect2D;
    return RADAR_TargetDetect::DetectRange;
}

bool RADAR_TargetDetect_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    return ProcessData(inputData);
}

bool RADAR_TargetDetect_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(CellSize)) {
        std::vector<bool> IsDetectData(1);
        std::vector<std::complex<double>> outputData(CellSize);
        std::vector<int> RangeBinIndexData(1);
        std::vector<int> FreqBinIndexData(1);

        // CFAR检测
        SystemVueModelBuilder::Matrix<double> LeadingWindow(1, ReferenceCell);
        SystemVueModelBuilder::Matrix<double> LaggingWindow(1, ReferenceCell);
        for (int i = 0; i < CellSize; i++)
        {
            // 获取当前参考窗（拼接法）
            for (int n = 0; n < ReferenceCell; n++)
            {
                int LeadingWindowIndex = i + n - GuardCell - ReferenceCell;
                int LaggingWindowIndex = i + n + GuardCell + 1;
                LeadingWindow(n) = std::abs(inputData[LeadingWindowIndex < 0 ? LeadingWindowIndex + CellSize : LeadingWindowIndex]);
                LaggingWindow(n) = std::abs(inputData[LaggingWindowIndex >= CellSize ? LaggingWindowIndex - CellSize : LaggingWindowIndex]);
            }

            double LeadingAvg = 0;
            double LaggingAvg = 0;
            for (int n = 0; n < ReferenceCell; n++)
            {
                LeadingAvg += LeadingWindow(n);
                LaggingAvg += LaggingWindow(n);
            }
            LeadingAvg /= ReferenceCell;
            LaggingAvg /= ReferenceCell;

            // CFAR门限因子计算
            double ThresholdFactor = 2 * ReferenceCell*(pow(FalseAlarmProbability, -1.0 / (2 * ReferenceCell)) - 1);

            // CFAR门限(CA-CFAR)
            Threshold = ThresholdFactor * (LeadingAvg + LaggingAvg) / 2;

            // 门限比较输出
            if (std::abs(inputData[i]) > Threshold)
            {
                outputData[i] = inputData[i];
                DetectStatus = true;
            }
            else
            {
                outputData[i] = 0;
            }
        }

        // 若检出目标，则输出目标的距离维与速度维索引
        if (DetectStatus)
        {
            // 快时间维求距离量
            double maxValue = 0.0;
            int maxIndex = 0;
            for (int i = 0; i < CellSize; i++)
            {
                if (std::abs(inputData[i]) > maxValue)
                {
                    maxValue = std::abs(inputData[i]);
                    maxIndex = i;
                }
            }
            // 距离维输出
            RangeBinIndexData[0] = fmod(maxIndex, PRINum);

            // 慢时间维求速度量
            int fmaxIndex = std::floor(maxIndex / PRINum);
            // 速度维输出
            FreqBinIndexData[0] = fmaxIndex;
        }
        else
        {
            RangeBinIndexData[0] = 0;
            FreqBinIndexData[0] = 0;
        }

        // 输出检测状态
        IsDetectData[0] = DetectStatus;
        // 每个Run重置检测状态
        DetectStatus = false;
        for(const auto& val : outputData) m_outputQueue.push(val);
        m_IsDetectQueue.push(IsDetectData[0]);
        m_RangeBinIndexQueue.push(RangeBinIndexData[0]);
        m_FreqBinIndexQueue.push(FreqBinIndexData[0]);
        if(!m_IsDetectQueue.empty() && !m_RangeBinIndexQueue.empty() && !m_FreqBinIndexQueue.empty()) {

            bool IsDetectValue = m_IsDetectQueue.front();
            m_IsDetectQueue.pop();
            WriteOutputData("IsDetect", std::vector<bool>{IsDetectValue});
            m_lastdetect = IsDetectValue;

            int RangeBinIndexValue = m_RangeBinIndexQueue.front();
            m_RangeBinIndexQueue.pop();
            WriteOutputData("RangeBinIndex", std::vector<int>{RangeBinIndexValue});
            m_lastrangeIndex = RangeBinIndexValue;

            int FreqBinIndexValue = m_FreqBinIndexQueue.front();
            m_FreqBinIndexQueue.pop();
            WriteOutputData("FreqBinIndex", std::vector<int>{FreqBinIndexValue});
            m_lastfreqIndex = FreqBinIndexValue;
        }
    }

    if(!m_outputQueue.empty()) {
        std::complex<double> outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        WriteOutputData("output", std::vector<std::complex<double>>{outputValue});
        m_lastOutput = outputValue;
        m_inputBuffer.clear();
        qDebug() << "[RADAR_TargetDetect_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << outputValue.imag();
    }
    return true;
}

bool RADAR_TargetDetect_Block::ProcessData(std::vector<std::complex<double> > inputData)
{
    std::vector<bool> IsDetectData(1);
    std::vector<std::complex<double>> outputData(CellSize);
    std::vector<int> RangeBinIndexData(1);
    std::vector<int> FreqBinIndexData(1);

    // CFAR检测
    SystemVueModelBuilder::Matrix<double> LeadingWindow(1, ReferenceCell);
    SystemVueModelBuilder::Matrix<double> LaggingWindow(1, ReferenceCell);
    for (int i = 0; i < CellSize; i++)
    {
        // 获取当前参考窗（拼接法）
        for (int n = 0; n < ReferenceCell; n++)
        {
            int LeadingWindowIndex = i + n - GuardCell - ReferenceCell;
            int LaggingWindowIndex = i + n + GuardCell + 1;
            LeadingWindow(n) = std::abs(inputData[LeadingWindowIndex < 0 ? LeadingWindowIndex + CellSize : LeadingWindowIndex]);
            LaggingWindow(n) = std::abs(inputData[LaggingWindowIndex >= CellSize ? LaggingWindowIndex - CellSize : LaggingWindowIndex]);
        }

        double LeadingAvg = 0;
        double LaggingAvg = 0;
        for (int n = 0; n < ReferenceCell; n++)
        {
            LeadingAvg += LeadingWindow(n);
            LaggingAvg += LaggingWindow(n);
        }
        LeadingAvg /= ReferenceCell;
        LaggingAvg /= ReferenceCell;

        // CFAR门限因子计算
        double ThresholdFactor = 2 * ReferenceCell*(pow(FalseAlarmProbability, -1.0 / (2 * ReferenceCell)) - 1);

        // CFAR门限(CA-CFAR)
        Threshold = ThresholdFactor * (LeadingAvg + LaggingAvg) / 2;

        // 门限比较输出
        if (std::abs(inputData[i]) > Threshold)
        {
            outputData[i] = inputData[i];
            DetectStatus = true;
        }
        else
        {
            outputData[i] = 0;
        }
    }

    // 若检出目标，则输出目标的距离维与速度维索引
    if (DetectStatus)
    {
        // 快时间维求距离量
        double maxValue = 0.0;
        int maxIndex = 0;
        for (int i = 0; i < CellSize; i++)
        {
            if (std::abs(inputData[i]) > maxValue)
            {
                maxValue = std::abs(inputData[i]);
                maxIndex = i;
            }
        }
        // 距离维输出
        RangeBinIndexData[0] = fmod(maxIndex, PRINum);

        // 慢时间维求速度量
        int fmaxIndex = std::floor(maxIndex / PRINum);
        // 速度维输出
        FreqBinIndexData[0] = fmaxIndex;
    }
    else
    {
        RangeBinIndexData[0] = 0;
        FreqBinIndexData[0] = 0;
    }

    // 输出检测状态
    IsDetectData[0] = DetectStatus;
    // 每个Run重置检测状态
    DetectStatus = false;

    WriteOutputData("IsDetect", IsDetectData);
    WriteOutputData("output", outputData);
    WriteOutputData("RangeBinIndex", RangeBinIndexData);
    WriteOutputData("FreqBinIndex", FreqBinIndexData);

    return true;
}

void RADAR_TargetDetect_Block::SetParameters()
{
    if(!m_radar) return;
    m_radar->PRI_Or_WaveGate = PRI_Or_WaveGate;
    m_radar->DetectType = DetectType;
    m_radar->FalseAlarmProbability = FalseAlarmProbability;
    m_radar->ReferenceCell = ReferenceCell;
    m_radar->GuardCell = GuardCell;
    m_radar->FreqChannelNum = FreqChannelNum;
    m_radar->SampleRate = SampleRate;
}

bool RADAR_TargetDetect_Block::ModelSetup()
{
    bool bStatus = true;

    // 参数校验
    if (PRI_Or_WaveGate <= 0)
    {
        LOG_ERROR("PRI_Or_WaveGate must be > 0");
        bStatus = false;
    }
    //if (SampleNumForEstimateNoise <= 0)
    //{
    //	LOG_ERROR("SampleNumForEstimateNoise must be > 0");
    //	bStatus = false;
    //}
    if (FalseAlarmProbability <= 0 || FalseAlarmProbability > 1)
    {
        LOG_ERROR("FalseAlarmProbability must be > 0 and <= 1");
        bStatus = false;
    }
    if (ReferenceCell <= 0)
    {
        LOG_ERROR("ReferenceCell must be > 0");
        bStatus = false;
    }
    if (GuardCell <= 0)
    {
        LOG_ERROR("GuardCell must be > 0");
        bStatus = false;
    }
    //if (Coef1 <= 0 || Coef2 <= 0 || Coef1 + Coef2 != 1)
    //{
    //	LOG_ERROR("Coef1 and Coef2 must be: Coef1 > 0, Coef2 > 0, Coef1 + Coef2 = 1");
    //	bStatus = false;
    //}
    //if (Coef <= 0)
    //{
    //	LOG_ERROR("Coef must be > 0");
    //	bStatus = false;
    //}
    //if (FreqChannelNum <= 0)
    //{
    //	LOG_ERROR("FreqChannelNum must be > 0");
    //	bStatus = false;
    //}
    if (SampleRate <= 0)
    {
        LOG_ERROR("SampleRate must be > 0");
        bStatus = false;
    }

    // 设置端口速率
    PRINum = PRI_Or_WaveGate * SampleRate;
    CellSize = DetectType ? PRINum * FreqChannelNum : PRINum;

    // 检测状态初始化
    DetectStatus = false;

    return bStatus;
}

