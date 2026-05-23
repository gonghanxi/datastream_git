#include "Polynomial_Block.h"

Polynomial_Block::Polynomial_Block(const std::string &name)
    :Block(name)
{

}

bool Polynomial_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Polynomial_Block::Run()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(inputData.size());

    int order = Coefficients.NumElements();
    double	result = 0.0;
    double	term = 1.0; // 0次项

    // 阶数从低到高，依次累加各项
    for (int i = 0; i < order; i++)
    {
        result += Coefficients(i) * term;
        term *= inputData[0];
    }

    outputData[0] = result;
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Polynomial_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Polynomial = std::make_unique<Polynomial>();
    SetDefaultParameters();
    try{ Coefficients = ParseStringToMatrix<double>(getParameter("Coefficients").Value); } catch(...) {}
    SetParameters();
    AddInputPort("input", m_Polynomial->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Polynomial->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void Polynomial_Block::SetParameters()
{
    if(!m_Polynomial) return;
    m_Polynomial->Coefficients = Coefficients;
}

void Polynomial_Block::SetDefaultParameters()
{
    Coefficients.Resize(1,2);
    Coefficients(0,0) = 0;
    Coefficients(0,1) = 1;
}
