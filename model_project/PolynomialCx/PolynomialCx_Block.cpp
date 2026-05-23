#include "PolynomialCx_Block.h"

PolynomialCx_Block::PolynomialCx_Block(const std::string &name)
    :Block(name)
{

}
bool PolynomialCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool PolynomialCx_Block::Run()
{
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    std::vector<std::complex<double>> outputData(inputData.size());

    qDebug() << "PolynomialCx_Block::Run - inputData real: " << inputData[0].real();
    qDebug() << "PolynomialCx_Block::Run - inputData imag: " << inputData[0].imag();

    int order = Coefficients.NumElements();
    qDebug() << "PolynomialCx_Block::Run - order: " << order;
//    qDebug() << "PolynomialCx_Block::Run - Coefficients NumDimensions: " << Coefficients.NumDimensions();
    std::complex<double>	result = 0.0;
    std::complex<double>	term = 1.0;

    for (int i = 0; i < order; i++)
    {
        qDebug() << QString("PolynomialCx_Block::Run - Coefficients real %1: %2").arg(i).arg(Coefficients(i).real());
        qDebug() << QString("PolynomialCx_Block::Run - Coefficients imag %1: %2").arg(i).arg(Coefficients(i).imag());
        result += Coefficients(i) * term;
        term *= inputData[0];
    }

    outputData[0] = result;
    qDebug() << "PolynomialCx_Block::Run - outputData real: " << outputData[0].real();
    qDebug() << "PolynomialCx_Block::Run - outputData imag: " << outputData[0].imag();
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool PolynomialCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Polynomial = std::make_unique<PolynomialCx>();
    SetDefaultParameters();
    try{ Coefficients = ParseStringToMatrix<std::complex<double>>(getParameter("Coefficients").Value); } catch(...) {}
    SetParameters();
    AddInputPort("input", m_Polynomial->input, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_Polynomial->output, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void PolynomialCx_Block::SetParameters()
{
    if(!m_Polynomial) return;
    m_Polynomial->Coefficients = Coefficients;
}

void PolynomialCx_Block::SetDefaultParameters()
{
    Coefficients.Resize(1,2);
    Coefficients(0,0) = 0;
    Coefficients(0,1) = 1;
}
