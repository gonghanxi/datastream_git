#include "ConvolveCx_Block.h"

ConvolveCx_Block::ConvolveCx_Block(const std::string& name)
    :Block(name)
{

}
bool ConvolveCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    if(!m_Convolve->Setup()) return false;
    depth_ = static_cast<std::size_t>(m_TruncationDepth);
    histA_.assign(depth_, std::complex<double>(0.0, 0.0));
    histB_.assign(depth_, std::complex<double>(0.0, 0.0));
    sampleCount_ = 0;
    return true;
}

bool ConvolveCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool ConvolveCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Convolve = std::make_unique<ConvolveCx>();
    SetDefaultParameters();
    try { m_TruncationDepth = std::stoi(getParameter("TruncationDepth").Value); } catch (...) {}
    SetParameters();

    AddInputPort("inA", m_Convolve->inA, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("inB", m_Convolve->inB, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("out", m_Convolve->out, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void ConvolveCx_Block::SetParameters()
{
    if(!m_Convolve) return;
    m_Convolve->TruncationDepth = m_TruncationDepth;
}

void ConvolveCx_Block::SetDefaultParameters()
{
    m_TruncationDepth = 256;
    histA_.clear();
    histB_.clear();
}

bool ConvolveCx_Block::DataStreamRun()
{
    std::string inAPort = GetInputPortName(0);
    std::string inBPort = GetInputPortName(1);
    std::string outPort = GetOutputPortName(0);

    std::vector<std::complex<double>> inAData = ReadInputData<std::complex<double>>(inAPort);
    std::vector<std::complex<double>> inBData = ReadInputData<std::complex<double>>(inBPort);
    std::vector<std::complex<double>> outputData(1);
    outputData.reserve(1);

    for (std::size_t k = depth_ - 1; k > 0; --k) {
        histB_[k] = histB_[k - 1];
    }
    histB_[0] = inBData[0];

    // 2) inA 按“样点序号”顺序写入一次，写满 depth_ 后保持不变
    //    histA_[0] = inA[0]
    //    histA_[1] = inA[1]
    //    ...
    if (sampleCount_ < depth_) {
        histA_[sampleCount_] = inAData[0];
    }

    // 3) 按内置帮助文档公式计算：
    //    out[n] = sum_{k=0}^{T-1} inA[k] * conj(inB[n-k])
    std::complex<double> acc(0.0, 0.0);

    const std::size_t terms =
        (sampleCount_ + 1 < depth_) ? (sampleCount_ + 1) : depth_;

    for (std::size_t k = 0; k < terms; ++k) {
        acc += histA_[k] * std::conj(histB_[k]);
    }

    outputData[0] = acc;

    ++sampleCount_;

    WriteOutputData(outPort, outputData);
    return true;
}

bool ConvolveCx_Block::TimeDrivenRun()
{
    std::string inAPort = GetInputPortName(0);
    std::string inBPort = GetInputPortName(1);
    std::string outPort = GetOutputPortName(0);

    std::vector<std::complex<double>> inAData = ReadInputData<std::complex<double>>(inAPort);
    std::vector<std::complex<double>> inBData = ReadInputData<std::complex<double>>(inBPort);
    std::vector<std::complex<double>> outputData(1);

    if(inAData.empty() || inBData.empty()) return true;

    m_inABuffer.push_back(inAData[0]);
    m_inBBuffer.push_back(inBData[0]);

    if(m_inABuffer.size() >= 1 && m_inBBuffer.size() >= 1) {
        for (std::size_t k = depth_ - 1; k > 0; --k) {
            histB_[k] = histB_[k - 1];
        }
        histB_[0] = m_inBBuffer[0];

        // 2) inA 按“样点序号”顺序写入一次，写满 depth_ 后保持不变
        //    histA_[0] = inA[0]
        //    histA_[1] = inA[1]
        //    ...
        if (sampleCount_ < depth_) {
            histA_[sampleCount_] = m_inABuffer[0];
        }

        // 3) 按内置帮助文档公式计算：
        //    out[n] = sum_{k=0}^{T-1} inA[k] * conj(inB[n-k])
        std::complex<double> acc(0.0, 0.0);

        const std::size_t terms =
            (sampleCount_ + 1 < depth_) ? (sampleCount_ + 1) : depth_;

        for (std::size_t k = 0; k < terms; ++k) {
            acc += histA_[k] * std::conj(histB_[k]);
        }

        outputData[0] = acc;

        ++sampleCount_;
        m_outputQueue.push(outputData[0]);

        if (!m_outputQueue.empty())
        {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inABuffer.clear();
            m_inBBuffer.clear();

            qDebug() << "[ConvolutionalCoder_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}
