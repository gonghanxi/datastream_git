#include "EnvFcChange_M_Block.h"

EnvFcChange_M_Block::EnvFcChange_M_Block(const std::string &name)
    :Block(name)
{

}

bool EnvFcChange_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool EnvFcChange_M_Block::Run()
{

    // 获取输入端口名称
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入矩阵数据（EnvelopeMatrix类型）
    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeMatrix>(inputPortName);
    if (inputData.empty()) {
        return true;  // 没有数据，等待下次调用
    }

    // 获取当前时间
    // t = startTime + count / samplingRate
    const double t = simulator_param.startTime + static_cast<double>(m_EnvFcChange_M->GetCount()) / simulator_param.samplingRate;

    // 获取输入矩阵（假设每次只处理一个矩阵）
    const SystemVueModelBuilder::EnvelopeMatrix& xin = inputData[0];

    // 创建输出矩阵
    SystemVueModelBuilder::EnvelopeMatrix yout;
    yout.Resize(xin.NumRows(), xin.NumColumns());

    // 参考原算法：fc_out = OutputFc（直接使用，不再做 >0 判断）
    double fc_in = GetInputPort(inputPortName)->getCharacterizationFrequency();
    double fc_out = m_OutputFc;

    if (!std::isfinite(fc_in) || fc_in < 0.0)
        fc_in = 0.0;

    if (!std::isfinite(fc_out) || fc_out < 0.0)
        fc_out = 0.0;

    const size_t numElements = xin.NumElements();

    // 参考原算法：fc_in == 0 且 fc_out > 0 → LPF + I/Q 提取
    if (fc_in == 0.0 && fc_out > 0.0)
    {
        resetLpfStateIfNeeded(numElements);

        const double bw = getEffectiveBandwidth();
        const double ts = getInputTimeStep();

        double alpha = 1.0;
        if (bw > 0.0 && ts > 0.0)
        {
            alpha = 1.0 - std::exp(-2.0 * EnvFcChange_M::kPI * bw * ts);
            if (alpha < 0.0) alpha = 0.0;
            if (alpha > 1.0) alpha = 1.0;
        }

        const double phase = 2.0 * EnvFcChange_M::kPI * fc_out * t;
        const double c = std::cos(phase);
        const double s = std::sin(phase);

        for (size_t i = 0; i < numElements; ++i)
        {
            const SystemVueModelBuilder::EnvelopeSignal& ein = xin(i);

            // fc_in = 0 时，输入按实通信号处理
            const std::complex<double> raw = ein.ConvertToNewFc(0.0, 0.0, t);
            const double x = raw.real();

            std::complex<double> mixed(2.0 * x * c, -2.0 * x * s);
            m_lpfState[i] = m_lpfState[i] + alpha * (mixed - m_lpfState[i]);

            CopyToEnvelopeSignal(m_lpfState[i], yout(i));
        }
    }
    else
    {
        // 参考原算法：fc_in != fc_out 或 fc_out == 0 时需要转换
        for (size_t i = 0; i < numElements; ++i)
        {
            const SystemVueModelBuilder::EnvelopeSignal& ein = xin(i);

            if (fc_in != fc_out || fc_out == 0.0)
            {
                std::complex<double> cx_new = ein.ConvertToNewFc(fc_in, fc_out, t);

                // 参考原算法：fc_out == 0 时将虚部置零
                if (fc_out == 0.0)
                {
                    cx_new = std::complex<double>(cx_new.real(), 0.0);
                }

                CopyToEnvelopeSignal(cx_new, yout(i));
            }
            else
            {
                // fc 相同且不为 0 时直接复制
                yout(i) = ein;
            }
        }
    }

    // 创建输出容器并写入
    std::vector<SystemVueModelBuilder::EnvelopeMatrix> outputData;
    outputData.push_back(yout);
    WriteOutputData(outputPortName, outputData);
    m_EnvFcChange_M->Advance();
    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fc_out);
    return true;
}

bool EnvFcChange_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_EnvFcChange_M = std::make_unique<EnvFcChange_M>();

    AddInputPort("input", m_EnvFcChange_M->input, 1, Block::DataType::MATRIX_ENVELOPE);
    AddOutputPort("output", m_EnvFcChange_M->output, 1, Block::DataType::MATRIX_ENVELOPE);

    SetDefaultParamters();

    try { m_OutputFc = std::stod(getParameter("OutputFc").Value); } catch (...) {}
    try { m_Bandwidth = std::stod(getParameter("Bandwidth").Value); } catch (...) {}
    simulator_param = getSimu();

    SetParameters();

    return true;
}

void EnvFcChange_M_Block::SetDefaultParamters()
{
    m_OutputFc = 0.0;
    m_Bandwidth = 0.0;
    m_lpfInitialized = false;
    m_lpfNumElements = 0;
    m_lpfState.clear();
}

void EnvFcChange_M_Block::SetParameters()
{
    if (!m_EnvFcChange_M) {
        return;
    }
    m_EnvFcChange_M->OutputFc = m_OutputFc;
    m_EnvFcChange_M->Bandwidth = m_Bandwidth;
    m_lpfInitialized = false;
    m_lpfNumElements = 0;
    m_lpfState.clear();
}

// ============================================================================
// LPF 辅助函数（参考原算法 input fc=0 分支）
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

double EnvFcChange_M_Block::getEffectiveBandwidth() const
{
    double bw = m_Bandwidth;

    if (!std::isfinite(bw) || bw < 0.0)
        bw = 0.0;

    if (bw == 0.0)
        bw = m_OutputFc;

    if (!std::isfinite(bw) || bw < 0.0)
        bw = 0.0;

    return bw;
}

double EnvFcChange_M_Block::getInputTimeStep() const
{
    double ts = simulator_param.time_Interval;

    if (!std::isfinite(ts) || ts <= 0.0)
    {
        const double fs = simulator_param.samplingRate;
        if (std::isfinite(fs) && fs > 0.0)
            ts = 1.0 / fs;
    }

    if (!std::isfinite(ts) || ts <= 0.0)
        ts = 1.0;

    return ts;
}
