#include "CrossCorr_Block.h"
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
CrossCorr_Block::CrossCorr_Block(const std::string &name)
    :Block(name)
{

}

bool CrossCorr_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool CrossCorr_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CrossCorr_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_CrossCorr = std::make_unique<CrossCorr>();
    SetDefaultParameters();
    try { m_CorrelationType = ConvertStringToCorrelationTypeEnum(getParameter("CorrelationType").Value); } catch (...) {}
    try { m_Normalization = ConvertStringToNormalizationEnum(getParameter("Normalization").Value); } catch (...) {}
    try { m_CorrelationLength = std::stoi(getParameter("CorrelationLength").Value); } catch (...) {}
    try { m_StartLag = std::stoi(getParameter("StartLag").Value); } catch (...) {}
    try { m_StopLag = std::stoi(getParameter("StopLag").Value); } catch (...) {}
    SetParameters();

    if(!m_CrossCorr->Setup()) return false;

    AddInputPort("input", m_CrossCorr->input, static_cast<size_t>(m_CorrelationLength), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("input2", m_CrossCorr->input2, static_cast<size_t>(m_CorrelationLength), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_CrossCorr->output, static_cast<size_t>(m_StopLag - m_StartLag + 1), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("delay", m_CrossCorr->delay, 1, DataType::CIRCULAR_BUFFER_INT);
    return true;
}

void CrossCorr_Block::SetParameters()
{
    if(!m_CrossCorr) return;
    m_CrossCorr->CorrelationType = m_CorrelationType;
    m_CrossCorr->CorrelationLength = m_CorrelationLength;
    m_CrossCorr->StartLag = m_StartLag;
    m_CrossCorr->StopLag = m_StopLag;
    m_CrossCorr->Normalization = m_Normalization;
}

CrossCorr::CorrelationTypeEnum CrossCorr_Block::ConvertStringToCorrelationTypeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "noncircular" || lower == "0") {
        return CrossCorr::NonCircular;
    }
    if (lower == "circular" || lower == "1") {
        return CrossCorr::Circular;
    }
    return CrossCorr::Circular;
}

CrossCorr::NormalizationEnum CrossCorr_Block::ConvertStringToNormalizationEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "none" || lower == "0") {
        return CrossCorr::None;
    }
    if (lower == "unbiased" || lower == "1") {
        return CrossCorr::UnBiased;
    }
    if (lower == "biased" || lower == "2") {
        return CrossCorr::Biased;
    }
    return CrossCorr::None;
}

void CrossCorr_Block::SetDefaultParameters()
{
    m_CorrelationType = CrossCorr::NonCircular;
    m_CorrelationLength = 500;
    m_StartLag = -50;
    m_StopLag = 50;
    m_Normalization = CrossCorr::None;
}

bool CrossCorr_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string input2Port = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);
    std::string delayPort = GetOutputPortName(1);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<double> input2Data = ReadInputData<double>(input2Port);
    std::vector<double> outputData(m_StopLag - m_StartLag + 1);
    std::vector<double> delayData(1);
    outputData.reserve(m_StopLag - m_StartLag + 1);
    delayData.reserve(1);
    const int N = m_CorrelationLength;
    const int kStart = m_StartLag;
    const int kStop = m_StopLag;
    const int numLags = kStop - kStart + 1;

    if (N <= 0 || numLags <= 0)
    {
        LOG_ERROR("Invalid parameters in Run().");
        return false;
    }

    std::vector<double> x(static_cast<std::size_t>(N));
    std::vector<double> y(static_cast<std::size_t>(N));

    for (int n = 0; n < N; ++n)
    {
        x[static_cast<std::size_t>(n)] = inputData[static_cast<std::size_t>(n)];
        y[static_cast<std::size_t>(n)] = input2Data[static_cast<std::size_t>(n)];
    }

    double bestMetric = -1.0;
    int    bestLag = 0;

    for (int k = kStart; k <= kStop; ++k)
    {
        double sum = 0.0;

        if (m_CorrelationType == CrossCorr::NonCircular)
        {
            int iStart = (k >= 0) ? 0 : -k;
            int iEnd = (k >= 0) ? N - k : N;

            for (int i = iStart; i < iEnd; ++i)
            {
                sum += x[static_cast<std::size_t>(i)]
                    * y[static_cast<std::size_t>(i + k)];
            }

            if (m_Normalization == CrossCorr::UnBiased)
            {
                const int denom = N - std::abs(k);
                if (denom > 0)
                    sum /= static_cast<double>(denom);
            }
            else if (m_Normalization == CrossCorr::Biased)
            {
                sum /= static_cast<double>(N);
            }
        }
        else
        {
            for (int i = 0; i < N; ++i)
            {
                int j = i + k;
                j = ((j % N) + N) % N;

                sum += x[static_cast<std::size_t>(i)]
                    * y[static_cast<std::size_t>(j)];
            }

            if (m_Normalization == CrossCorr::UnBiased || m_Normalization == CrossCorr::Biased)
            {
                sum /= static_cast<double>(N);
            }
        }

        const int outIdx = k - kStart;
        outputData[static_cast<std::size_t>(outIdx)] = sum;

        const double metric = std::fabs(sum);
        if (metric > bestMetric)
        {
            bestMetric = metric;
            bestLag = k;
        }
    }

    delayData[0U] = bestLag;

    WriteOutputData(outputPort, outputData);
    WriteOutputData(delayPort, delayData);
    return true;
}

bool CrossCorr_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string input2Port = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);
    std::string delayPort = GetOutputPortName(1);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<double> input2Data = ReadInputData<double>(input2Port);
    std::vector<double> outputData(m_StopLag - m_StartLag + 1);
    std::vector<double> delayData(1);
    outputData.reserve(m_StopLag - m_StartLag + 1);
    delayData.reserve(1);
    if(inputData.empty() || input2Data.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    for(const auto& val : input2Data) m_input2Buffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_CorrelationLength)
            && m_input2Buffer.size() >= static_cast<size_t>(m_CorrelationLength)) {
        const int N = m_CorrelationLength;
        const int kStart = m_StartLag;
        const int kStop = m_StopLag;
        const int numLags = kStop - kStart + 1;

        if (N <= 0 || numLags <= 0)
        {
            LOG_ERROR("Invalid parameters in Run().");
            return false;
        }

        std::vector<double> x(static_cast<std::size_t>(N));
        std::vector<double> y(static_cast<std::size_t>(N));

        for (int n = 0; n < N; ++n)
        {
            x[static_cast<std::size_t>(n)] = m_inputBuffer[static_cast<std::size_t>(n)];
            y[static_cast<std::size_t>(n)] = m_input2Buffer[static_cast<std::size_t>(n)];
        }

        double bestMetric = -1.0;
        int    bestLag = 0;

        for (int k = kStart; k <= kStop; ++k)
        {
            double sum = 0.0;

            if (m_CorrelationType == CrossCorr::NonCircular)
            {
                int iStart = (k >= 0) ? 0 : -k;
                int iEnd = (k >= 0) ? N - k : N;

                for (int i = iStart; i < iEnd; ++i)
                {
                    sum += x[static_cast<std::size_t>(i)]
                        * y[static_cast<std::size_t>(i + k)];
                }

                if (m_Normalization == CrossCorr::UnBiased)
                {
                    const int denom = N - std::abs(k);
                    if (denom > 0)
                        sum /= static_cast<double>(denom);
                }
                else if (m_Normalization == CrossCorr::Biased)
                {
                    sum /= static_cast<double>(N);
                }
            }
            else
            {
                for (int i = 0; i < N; ++i)
                {
                    int j = i + k;
                    j = ((j % N) + N) % N;

                    sum += x[static_cast<std::size_t>(i)]
                        * y[static_cast<std::size_t>(j)];
                }

                if (m_Normalization == CrossCorr::UnBiased || m_Normalization == CrossCorr::Biased)
                {
                    sum /= static_cast<double>(N);
                }
            }

            const int outIdx = k - kStart;
            outputData[static_cast<std::size_t>(outIdx)] = sum;

            const double metric = std::fabs(sum);
            if (metric > bestMetric)
            {
                bestMetric = metric;
                bestLag = k;
            }
        }

        delayData[0U] = bestLag;
        // 将输出块中的每个样本逐个放入输出队列
        for (const auto& val : outputData)
        {
            m_outputQueue.push(val);
        }
        if (!m_outputQueue.empty())
        {
            bool outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<bool>{outputValue});
            WriteOutputData(GetInputPortName(1), delayData);
            m_lastOutput = outputValue;
            m_inputBuffer.clear();
            m_input2Buffer.clear();

            qDebug() << "[CrossCorr_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
}
