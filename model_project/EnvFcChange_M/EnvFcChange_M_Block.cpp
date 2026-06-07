#include "EnvFcChange_M_Block.h"

#include <cmath>

static constexpr double kPI = 3.14159265358979323846;

// ============================================================================
// 构造函数
// ============================================================================

EnvFcChange_M_Block::EnvFcChange_M_Block(const std::string& name)
    : Block(name)
    , m_OutputFc(0.0)
    , m_Bandwidth(0.0)
    , m_fc_in(0.0)
    , m_fc_out(0.0)
    , m_lpfInitialized(false)
    , m_lpfNumElements(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool EnvFcChange_M_Block::Setup()
{
    Block::Setup();

    // —— 从端口 reader 获取 fc_in（非 algo 内部 buffer）——
    m_fc_in = 0.0;
    {
        auto* reader = GetInputPort(GetInputPortName(0));
        if (reader && reader->hasCharacterizationFrequency())
            m_fc_in = reader->getCharacterizationFrequency();
    }
    if (!std::isfinite(m_fc_in) || m_fc_in < 0.0)
        m_fc_in = 0.0;

    m_fc_out = m_OutputFc;
    if (!std::isfinite(m_fc_out) || m_fc_out < 0.0)
        m_fc_out = 0.0;

    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(m_fc_out);

    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_lpfState.clear();
    m_lpfInitialized = false;
    m_lpfNumElements = 0;
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool EnvFcChange_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool EnvFcChange_M_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeMatrix>(inputPort);
    if (inputData.empty()) return true;

    const double t = m_simulatorParam.startTime
        + static_cast<double>(m_EnvFcChange_M->GetCount()) / m_simulatorParam.samplingRate;

    const EnvelopeMatrix& xin = inputData[0];
    EnvelopeMatrix yout;
    yout.Resize(xin.NumRows(), xin.NumColumns());

    const size_t numElements = xin.NumElements();

    // ============================================================
    // 分支一：input fc=0 且 OutputFc>0 —— LPF 路径
    // ============================================================
    if (m_fc_in == 0.0 && m_fc_out > 0.0)
    {
        resetLpfStateIfNeeded(numElements);

        const double bw = getEffectiveBandwidth();
        const double ts = m_simulatorParam.time_Interval;
        double alpha = 1.0;

        if (bw > 0.0 && ts > 0.0)
        {
            alpha = 1.0 - std::exp(-2.0 * kPI * bw * ts);
            if (alpha < 0.0) alpha = 0.0;
            if (alpha > 1.0) alpha = 1.0;
        }

        const double phase = 2.0 * kPI * m_fc_out * t;
        const double c = std::cos(phase);
        const double s = std::sin(phase);

        for (size_t i = 0; i < numElements; ++i)
        {
            const EnvelopeSignal& ein = xin(i);
            const std::complex<double> raw = ein.ConvertToNewFc(0.0, 0.0, t);
            const double x = raw.real();

            std::complex<double> mixed(2.0 * x * c, -2.0 * x * s);

            m_lpfState[i] = m_lpfState[i] + alpha * (mixed - m_lpfState[i]);

            CopyToEnvelopeSignal(m_lpfState[i], yout(i));
        }
    }
    // ============================================================
    // 分支二：input fc > 0
    // ============================================================
    else
    {
        for (size_t i = 0; i < numElements; ++i)
        {
            const EnvelopeSignal& ein = xin(i);

            if (m_fc_in != m_fc_out || m_fc_out == 0.0)
            {
                std::complex<double> cx_new = ein.ConvertToNewFc(m_fc_in, m_fc_out, t);

                if (m_fc_out == 0.0)
                    cx_new = std::complex<double>(cx_new.real(), 0.0);

                CopyToEnvelopeSignal(cx_new, yout(i));
            }
            else
            {
                yout(i) = ein;
            }
        }
    }

    m_EnvFcChange_M->Advance();

    std::vector<EnvelopeMatrix> outputData;
    outputData.push_back(yout);
    WriteOutputData(outputPort, outputData);
    GetOutputPort(outputPort)->setCharacterizationFrequency(m_fc_out);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool EnvFcChange_M_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeMatrix>(inputPort);
    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= 1)
    {
        const double t = m_simulatorParam.startTime
            + static_cast<double>(m_EnvFcChange_M->GetCount()) / m_simulatorParam.samplingRate;

        const EnvelopeMatrix& xin = m_inputBuffer[0];
        EnvelopeMatrix yout;
        yout.Resize(xin.NumRows(), xin.NumColumns());

        const size_t numElements = xin.NumElements();

        if (m_fc_in == 0.0 && m_fc_out > 0.0)
        {
            resetLpfStateIfNeeded(numElements);

            const double bw = getEffectiveBandwidth();
            const double ts = m_simulatorParam.time_Interval;
            double alpha = 1.0;

            if (bw > 0.0 && ts > 0.0)
            {
                alpha = 1.0 - std::exp(-2.0 * kPI * bw * ts);
                if (alpha < 0.0) alpha = 0.0;
                if (alpha > 1.0) alpha = 1.0;
            }

            const double phase = 2.0 * kPI * m_fc_out * t;
            const double c = std::cos(phase);
            const double s = std::sin(phase);

            for (size_t i = 0; i < numElements; ++i)
            {
                const EnvelopeSignal& ein = xin(i);
                const std::complex<double> raw = ein.ConvertToNewFc(0.0, 0.0, t);
                const double x = raw.real();

                std::complex<double> mixed(2.0 * x * c, -2.0 * x * s);

                m_lpfState[i] = m_lpfState[i] + alpha * (mixed - m_lpfState[i]);

                CopyToEnvelopeSignal(m_lpfState[i], yout(i));
            }
        }
        else
        {
            for (size_t i = 0; i < numElements; ++i)
            {
                const EnvelopeSignal& ein = xin(i);

                if (m_fc_in != m_fc_out || m_fc_out == 0.0)
                {
                    std::complex<double> cx_new = ein.ConvertToNewFc(m_fc_in, m_fc_out, t);

                    if (m_fc_out == 0.0)
                        cx_new = std::complex<double>(cx_new.real(), 0.0);

                    CopyToEnvelopeSignal(cx_new, yout(i));
                }
                else
                {
                    yout(i) = ein;
                }
            }
        }

        m_EnvFcChange_M->Advance();

        m_outputQueue.push(yout);
        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        EnvelopeMatrix outMat = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<EnvelopeMatrix> outputData;
        outputData.push_back(outMat);
        WriteOutputData(outputPort, outputData);
        GetOutputPort(outputPort)->setCharacterizationFrequency(m_fc_out);
    }

    return true;
}

// ============================================================================
// resetLpfStateIfNeeded
// ============================================================================

void EnvFcChange_M_Block::resetLpfStateIfNeeded(size_t numElements)
{
    if (!m_lpfInitialized || m_lpfNumElements != numElements)
    {
        m_lpfState.assign(numElements, std::complex<double>(0.0, 0.0));
        m_lpfNumElements = numElements;
        m_lpfInitialized = true;
    }
}

// ============================================================================
// getEffectiveBandwidth
// ============================================================================

double EnvFcChange_M_Block::getEffectiveBandwidth() const
{
    double bw = m_Bandwidth;

    if (!std::isfinite(bw) || bw < 0.0)
        bw = 0.0;

    if (bw == 0.0)
        bw = m_fc_out;

    if (!std::isfinite(bw) || bw < 0.0)
        bw = 0.0;

    return bw;
}

// ============================================================================
// Initialize
// ============================================================================

bool EnvFcChange_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_EnvFcChange_M = std::make_unique<EnvFcChange_M>();

    SetDefaultParameters();
    try { m_OutputFc  = std::stod(getParameter("OutputFc").Value);  } catch (...) {}
    try { m_Bandwidth = std::stod(getParameter("Bandwidth").Value); } catch (...) {}
    m_simulatorParam = getSimu();
    SetParameters();

    AddInputPort("input",  m_EnvFcChange_M->input,  1, Block::DataType::MATRIX_ENVELOPE);
    AddOutputPort("output", m_EnvFcChange_M->output, 1, Block::DataType::MATRIX_ENVELOPE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void EnvFcChange_M_Block::SetDefaultParameters()
{
    m_OutputFc  = 0.0;
    m_Bandwidth = 0.0;
}

void EnvFcChange_M_Block::SetParameters()
{
    if (!m_EnvFcChange_M) return;
    m_EnvFcChange_M->OutputFc  = m_OutputFc;
    m_EnvFcChange_M->Bandwidth = m_Bandwidth;
}
