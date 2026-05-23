#include "AddNDensity_Block.h"
#include <random>

AddNDensity_Block::AddNDensity_Block(const std::string &name)
    :Block(name)
{

}


bool AddNDensity_Block::Setup()
{
    Block::Setup();
    if (NDensity < 0)
    {
        LOG_ERROR("NDensity must be >= 0");
        return false;
    }
    if (RefR <= 0)
    {
        LOG_ERROR("RefR must be > 0");
        return false;
    }
    return true;
}

bool AddNDensity_Block::Run()
{
    return DataStreamRun();
}

bool AddNDensity_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_addn = std::make_unique<AddNDensity>();

    SetDefaultParamters();

    try {
        NDensity = std::stod(getParameter("NDensity").Value);
        RefR = std::stod(getParameter("RefR").Value);
    } catch (...) {

    }

    SetParameters();

    AddInputPort("input", m_addn->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_addn->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

void AddNDensity_Block::SetParameters()
{
    if(!m_addn) return;
    m_addn->NDensity = NDensity;
    m_addn->RefR = RefR;
}
void AddNDensity_Block::SetDefaultParamters()
{
    NDensity = 4.00388587e-21;
    RefR = 50;
}

bool AddNDensity_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* reader = GetInputPort(inputPort);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    std::vector<EnvelopeSignal> outputData(1);

    double SampleRate = getSimu().samplingRate;

    // 射频信号
    if (reader->hasCharacterizationFrequency())
    {
        double BW = SampleRate / 2;
        double StdDev = std::sqrt(NDensity * BW * RefR);

        // 生成复高斯噪声
        std::random_device rd;	// 随机器
        std::mt19937 gen(rd()); // 梅森旋转生成种子
        std::normal_distribution<double>	dNRe(0, StdDev);
        std::normal_distribution<double>	dNIm(0, StdDev);

        std::complex<double> GaussianNoiseCx(dNRe(gen), dNIm(gen));

        outputData[0] = inputData[0].complex() + GaussianNoiseCx;
    }

    // 基带信号
    else
    {
        double BW = SampleRate;
        double StdDev = std::sqrt(NDensity * BW * RefR);

        // 生成高斯噪声
        std::random_device rd;	// 随机器
        std::mt19937 gen(rd()); // 梅森旋转生成种子
        std::normal_distribution<double>	dN(0, StdDev);

        outputData[0] = inputData[0].real() + dN(gen);
    }
    WriteOutputData(outputPort, outputData);

    return true;
}
