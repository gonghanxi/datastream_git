#include "AutoCorr_Block.h"
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
AutoCorr_Block::AutoCorr_Block(const std::string &name)
    :Block(name)
{

}

bool AutoCorr_Block::Setup()
{
    Block::Setup();
    if(!m_AutoCorr->Setup()) return false;
    while(!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

bool AutoCorr_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<double> outputData(m_numLags);
    outputData.reserve(m_numLags);

    const int N = m_CorrelationLength;
    const int numLags = m_numLags;

    if (N <= 0 || numLags <= 0) {
        LOG_ERROR("AutoCorr: invalid internal state (N <= 0 or numLags <= 0).");
        return false;
    }

    for (int n = 0; n < N; ++n) {
        m_samples[static_cast<size_t>(n)] = inputData[static_cast<size_t>(n)];
    }

    for (int idx = 0; idx < numLags; ++idx) {
        const int lag = m_StartLag + idx;

        double r = 0.0;
        if (m_CorrelationType == AutoCorr::Circular) {
            r = circularAutoCorrelation(lag);
        }
        else {
            r = nonCircularAutoCorrelation(lag);
        }

        switch (m_Normalization) {
        case AutoCorr::None:
            break;

        case AutoCorr::UnBiased:
            if (m_CorrelationType == AutoCorr::NonCircular) {
                {
                    const int denom = N - std::abs(lag);
                    if (denom > 0) {
                        r /= static_cast<double>(denom);
                    }
                    else {
                        r = 0.0;
                    }
                }
            }
            else {
                r /= static_cast<double>(N);
            }
            break;

        case AutoCorr::Biased:
            r /= static_cast<double>(N);
            break;
        }

        outputData[static_cast<size_t>(idx)] = r;
    }
    WriteOutputData(outputPort, outputData);

    return true;
}

bool AutoCorr_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    if(inputData.empty()) return true;
    for(size_t i = 0; i < inputData.size(); i++) {
        m_inputBuffer.push_back(inputData.size());
    }
    if(m_inputBuffer.size() >= static_cast<size_t>(m_CorrelationLength)) {
        const int N = m_CorrelationLength;
        const int numLags = m_numLags;
        std::vector<double> outputData(m_numLags);
        if (N <= 0 || numLags <= 0) {
            LOG_ERROR("AutoCorr: invalid internal state (N <= 0 or numLags <= 0).");
            return false;
        }

        for (int n = 0; n < N; ++n) {
            m_samples[static_cast<size_t>(n)] = m_inputBuffer[static_cast<size_t>(n)];
        }

        for (int idx = 0; idx < numLags; ++idx) {
            const int lag = m_StartLag + idx;

            double r = 0.0;
            if (m_CorrelationType == AutoCorr::Circular) {
                r = circularAutoCorrelation(lag);
            }
            else {
                r = nonCircularAutoCorrelation(lag);
            }

            switch (m_Normalization) {
            case AutoCorr::None:
                break;

            case AutoCorr::UnBiased:
                if (m_CorrelationType == AutoCorr::NonCircular) {
                    {
                        const int denom = N - std::abs(lag);
                        if (denom > 0) {
                            r /= static_cast<double>(denom);
                        }
                        else {
                            r = 0.0;
                        }
                    }
                }
                else {
                    r /= static_cast<double>(N);
                }
                break;

            case AutoCorr::Biased:
                r /= static_cast<double>(N);
                break;
            }

            outputData[static_cast<size_t>(idx)] = r;
        }

        for (const auto& val : outputData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
        if (!m_outputQueue.empty())
        {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}

bool AutoCorr_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool AutoCorr_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_AutoCorr = std::make_unique<AutoCorr>();
    SetDefaultParameters();
    try { m_CorrelationType = ConvertStringToCorrelationType(getParameter("CorrelationType").Value); } catch (...) {}
    try { m_Normalization = ConvertStringToNormalization(getParameter("Normalization").Value); } catch (...) {}
    try { m_CorrelationLength = std::stoi(getParameter("CorrelationLength").Value); } catch (...) {}
    try { m_StartLag = std::stoi(getParameter("StartLag").Value); } catch (...) {}
    try { m_StopLag = std::stoi(getParameter("StopLag").Value); } catch (...) {}
    SetParameters();
    if(!ModelsSetup()) return false;
    AddInputPort("input", m_AutoCorr->input, static_cast<size_t>(m_CorrelationLength), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_AutoCorr->output, static_cast<size_t>(m_numLags), DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

void AutoCorr_Block::SetParameters()
{
    if(!m_AutoCorr) return;
    m_AutoCorr->m_CorrelationType = m_CorrelationType;
    m_AutoCorr->m_Normalization = m_Normalization;
    m_AutoCorr->CorrelationLength = m_CorrelationLength;
    m_AutoCorr->StartLag = m_StartLag;
    m_AutoCorr->StopLag = m_StopLag;
}

AutoCorr::CorrelationType AutoCorr_Block::ConvertStringToCorrelationType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "noncircular" || lower == "0") {
        return AutoCorr::NonCircular;
    }
    if (lower == "circular" || lower == "1") {
        return AutoCorr::Circular;
    }
    return AutoCorr::Circular;
}

AutoCorr::Normalization AutoCorr_Block::ConvertStringToNormalization(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "noncircular" || lower == "0") {
        return AutoCorr::None;
    }
    if (lower == "circular" || lower == "1") {
        return AutoCorr::UnBiased;
    }
    if (lower == "circular" || lower == "2") {
        return AutoCorr::Biased;
    }
    return AutoCorr::None;
}

void AutoCorr_Block::SetDefaultParameters()
{
    m_CorrelationType = AutoCorr::Circular;
    m_Normalization = AutoCorr::None;
    m_CorrelationLength = 500;
    m_StartLag = -50;
    m_StopLag = 50;
}

bool AutoCorr_Block::ModelsSetup()
{
    if (m_CorrelationLength <= 0) {
        LOG_ERROR("AutoCorr: CorrelationLength must be > 0.");
        return false;
    }

    if (m_StopLag < m_StartLag) {
        LOG_ERROR("AutoCorr: StopLag must be >= StartLag.");
        return false;
    }

    m_numLags = m_StopLag - m_StartLag + 1;
    if (m_numLags <= 0) {
        LOG_ERROR("AutoCorr: StopLag - StartLag + 1 must be > 0.");
        return false;
    }

    const int N = m_CorrelationLength;
    if (m_StartLag < -(N - 1) || m_StopLag >(N - 1)) {
        LOG_WARN("AutoCorr: |lag| larger than CorrelationLength-1; "
            "NonCircular estimate will be based on fewer overlapping samples.");
    }

    m_samples.assign(static_cast<size_t>(N), 0.0);

    return true;
}

double AutoCorr_Block::nonCircularAutoCorrelation(int lag)
{
    const int N = m_CorrelationLength;
    double sum = 0.0;

    for (int i = 0; i < N; ++i) {
        const int j = i + lag;
        if (0 <= j && j < N) {
            sum += m_samples[static_cast<size_t>(i)] *
                m_samples[static_cast<size_t>(j)];
        }
    }

    return sum;
}

double AutoCorr_Block::circularAutoCorrelation(int lag)
{
    const int N = m_CorrelationLength;
    if (N <= 0) {
        return 0.0;
    }

    int k = lag % N;
    if (k < 0) {
        k += N;
    }

    double sum = 0.0;
    for (int i = 0; i < N; ++i) {
        int j = i + k;
        if (j >= N) {
            j -= N;
        }

        sum += m_samples[static_cast<size_t>(i)] *
            m_samples[static_cast<size_t>(j)];
    }

    return sum;
}


