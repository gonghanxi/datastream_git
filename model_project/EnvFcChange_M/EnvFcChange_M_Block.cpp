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

    // 获取输入和输出的特征频率
    double fc_in = GetInputPort(inputPortName)->getCharacterizationFrequency();
    double fc_out = m_OutputFc > 0.0 ? m_OutputFc : fc_in;

    if (!std::isfinite(fc_out) || fc_out < 0.0)
        fc_out = 0.0;

    // 根据频率是否相同进行处理
    if (fc_in != fc_out && fc_in > 0.0 && fc_out > 0.0)
    {
        // 需要转换频率
        for (size_t i = 0; i < xin.NumElements(); ++i)
        {
            const SystemVueModelBuilder::EnvelopeSignal& ein = xin(i);

            // 调用 EnvelopeSignal 的 ConvertToNewFc 方法进行频率转换
            std::complex<double> cx_new = ein.ConvertToNewFc(fc_in, fc_out, t);

            // 复制到输出矩阵
            CopyToEnvelopeSignal(cx_new, yout(i));
        }
    }
    else
    {
        // 频率相同，直接复制
        for (size_t i = 0; i < xin.NumElements(); ++i)
        {
            yout(i) = xin(i);
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
}

void EnvFcChange_M_Block::SetParameters()
{
    if (!m_EnvFcChange_M) {
        return;
    }
    m_EnvFcChange_M->OutputFc = m_OutputFc;
    m_EnvFcChange_M->Bandwidth = m_Bandwidth;

}
