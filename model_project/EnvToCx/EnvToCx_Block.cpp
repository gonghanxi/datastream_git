#include "EnvToCx_Block.h"

EnvToCx_Block::EnvToCx_Block(const std::string &name)
    :Block(name)
{}

bool EnvToCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool EnvToCx_Block::Run()
{

    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputport = GetInputPort(inputPortName);

    Buffer* connectedBuffer = inputport->GetConnectedBuffer();
    if (!connectedBuffer) {
        return false;
    }

    //实际读取数据
    std::vector<SystemVueModelBuilder::EnvelopeSignal> inputData;

    // 先创建空 vector
    std::vector<SystemVueModelBuilder::EnvelopeSignal> tempData;

    // 直接调用，不使用 reserve
    bool readSuccess = inputport->ReadData(tempData);

    if (readSuccess && !tempData.empty()) {
        // 使用 swap 避免拷贝
        inputData.swap(tempData);
    } else {
        return false;
    }


    // 后续处理...
    if (inputData.empty()) {
        return false;
    }

    // 简单的数据处理
    std::vector<std::complex<double>> outputData;

    try {
        for (size_t i = 0; i < inputData.size(); i++) {
            outputData.push_back(inputData[i].complex());
        }


    } catch (const std::exception& e) {
        return false;
    }

    // 写入输出
    try {
        bool writeSuccess = WriteOutputData(GetOutputPortName(0), outputData);

        if (writeSuccess) {
        }

    } catch (const std::exception& e) {
        std::cout << "EXCEPTION during write: " << e.what() << std::endl;
    }
    return true;
}

bool EnvToCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_envtoCx = std::make_unique<EnvToCx>();

    AddInputPort("Env", m_envtoCx->Env, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("Cx", m_envtoCx->Cx, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("Fc", m_envtoCx->Fc, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

void EnvToCx_Block::SetDefaultParameters()
{

}

void EnvToCx_Block::UpdateCharacterizationFrequency()
{
    if(m_envtoCx) {
        m_envtoCx->PropagateCharacterizationFrequency();
    }
}
