#include "PolynomialInt_Block.h"

PolynomialInt_Block::PolynomialInt_Block(const std::string &name)
    :Block(name)
{

}
bool PolynomialInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool PolynomialInt_Block::Run()
{
    std::vector<int> inputData = ReadInputData<int>(GetInputPortName(0));
    std::vector<int> outputData(inputData.size());

    int order = Coefficients.NumElements();
    int	result = 0;
    int	term = 1;

    for (int i = 0; i < order; i++)
    {
        result += Coefficients(i) * term;
        term *= inputData[0];
    }
    outputData[0] = result;


    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool PolynomialInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Polynomial = std::make_unique<PolynomialInt>();
    SetDefaultParameters();
    try{ Coefficients = ParseStringToMatrix<int>(getParameter("Coefficients").Value); } catch(...) {}
    SetParameters();
    AddInputPort("input", m_Polynomial->input, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_Polynomial->output, 1, DataType::CIRCULAR_BUFFER_INT);
    return true;
}

void PolynomialInt_Block::SetParameters()
{
    if(!m_Polynomial) return;
    m_Polynomial->Coefficients = Coefficients;
}

void PolynomialInt_Block::SetDefaultParameters()
{
    Coefficients.Resize(1,2);
    Coefficients(0,0) = 0;
    Coefficients(0,1) = 1;
}
