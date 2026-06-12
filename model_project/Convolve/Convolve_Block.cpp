#include "Convolve_Block.h"

Convolve_Block::Convolve_Block(const std::string &name)
    :Block(name)
{

}

bool Convolve_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    if(!m_Convolve->Setup()) return false;
    return true;
}

bool Convolve_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Convolve_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Convolve = std::make_unique<Convolve>();
    SetDefaultParameters();
    try { m_TruncationDepth = std::stoi(getParameter("TruncationDepth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TruncationDepth', using default value."); }
    SetParameters();

    AddInputPort("inA", m_Convolve->inA, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("inB", m_Convolve->inB, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("out", m_Convolve->out, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void Convolve_Block::SetParameters()
{
    if(!m_Convolve) return;
    m_Convolve->TruncationDepth = m_TruncationDepth;
}

void Convolve_Block::SetDefaultParameters()
{
    m_TruncationDepth = 256;
    histA_.clear();
    histB_.clear();
    iter_ = 0;
}

bool Convolve_Block::DataStreamRun()
{
    std::string inAPort = GetInputPortName(0);
    std::string inBPort = GetInputPortName(1);
    std::string outPort = GetOutputPortName(0);

    std::vector<double> inAData = ReadInputData<double>(inAPort);
    std::vector<double> inBData = ReadInputData<double>(inBPort);
    std::vector<double> outputData(1);
    outputData.reserve(1);
    const double a_n = inAData[0];
    const double b_n = inBData[0];

    histA_.push_back(a_n);
    histB_.push_back(b_n);

    const unsigned long long n = iter_;
    const std::size_t lenA = histA_.size();
    const std::size_t lenB = histB_.size();

    const int maxTerms = m_TruncationDepth;

    double acc = 0.0;
    int    termsUsed = 0;

    for (unsigned long long k = 0; k <= n; ++k)
    {
        if (termsUsed >= maxTerms)
            break;

        const std::size_t idxA = static_cast<std::size_t>(k);
        const std::size_t idxB = static_cast<std::size_t>(n - k);

        if (idxA >= lenA || idxB >= lenB)
            continue;

        acc += histA_[idxA] * histB_[idxB];
        ++termsUsed;
    }

    outputData[0] = acc;

    ++iter_;
    WriteOutputData(outPort, outputData);
    return true;
}

bool Convolve_Block::TimeDrivenRun()
{
    std::string inAPort = GetInputPortName(0);
    std::string inBPort = GetInputPortName(1);
    std::string outPort = GetOutputPortName(0);

    std::vector<double> inAData = ReadInputData<double>(inAPort);
    std::vector<double> inBData = ReadInputData<double>(inBPort);
    std::vector<double> outputData(1);

    if(inAData.empty() || inBData.empty()) return true;

    m_inABuffer.push_back(inAData[0]);
    m_inBBuffer.push_back(inBData[0]);

    if(m_inABuffer.size() >= 1 && m_inBBuffer.size() >= 1) {
        const double a_n = m_inABuffer[0];
        const double b_n = m_inBBuffer[0];

        histA_.push_back(a_n);
        histB_.push_back(b_n);

        const unsigned long long n = iter_;
        const std::size_t lenA = histA_.size();
        const std::size_t lenB = histB_.size();

        const int maxTerms = m_TruncationDepth;

        double acc = 0.0;
        int    termsUsed = 0;

        for (unsigned long long k = 0; k <= n; ++k)
        {
            if (termsUsed >= maxTerms)
                break;

            const std::size_t idxA = static_cast<std::size_t>(k);
            const std::size_t idxB = static_cast<std::size_t>(n - k);

            if (idxA >= lenA || idxB >= lenB)
                continue;

            acc += histA_[idxA] * histB_[idxB];
            ++termsUsed;
        }

        outputData[0] = acc;

        ++iter_;
        m_outputQueue.push(outputData[0]);

        if (!m_outputQueue.empty())
        {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
            m_inABuffer.clear();
            m_inBBuffer.clear();

            qDebug() << "[ConvolutionalCoder_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}
