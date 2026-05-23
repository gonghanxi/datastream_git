#include "CxToEnv_M_Block.h"

CxToEnv_M_Block::CxToEnv_M_Block(const std::string &name)
    :Block(name)
{

}

bool CxToEnv_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool CxToEnv_M_Block::Run()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);
    std::vector<EnvelopeMatrix> outputData(1);

    //----------------读取数据---------------------
    auto inputData = ReadInputData<DComplexMatrix>(inputPortName);
    if(inputData.empty()) {
        return false;
    }

    //----------------数据处理---------------------
    int NRow = inputData[0].NumRows();
    int NCol = inputData[0].NumColumns();
    outputData[0].Resize(NRow, NCol);

    for (int row = 0; row < NRow; row++)
    {
        for (int col = 0; col < NCol; col++)
        {
            outputData[0](row, col) = inputData[0](row, col);
        }
    }
    //----------------数据处理---------------------

    //----------------写入数据---------------------
    WriteOutputData(outputPortName, outputData);
    Buffer* outputBuffer = GetOutputPort(outputPortName);
    BufferReader* fcReader = GetInputPort(GetInputPortName(1));
    if(fcReader->IsConnected()) {
        auto fcData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(GetInputPortName(1));
        outputBuffer->setCharacterizationFrequency(fcData[0].real());
    }
    else {
        outputBuffer->setCharacterizationFrequency(Fc);
    }
    return true;
}

bool CxToEnv_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_cx = std::make_unique<CxToEnv_M>();
    SetDefaultParameters();

    Fc = std::stod(getParameter("Fc").Value);

    SetParameters();

    AddInputPort("input", m_cx->input, 1, DataType::MATRIX_DCOMPLEX);
    AddInputPort("fc", m_cx->fc, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_cx->output, 1, DataType::MATRIX_ENVELOPE);

    return true;
}

void CxToEnv_M_Block::SetParameters()
{
    if(!m_cx) return;
    m_cx->Fc = Fc;
}

void CxToEnv_M_Block::SetDefaultParameters()
{
    Fc = 0.2e6;
}
