#include "CxToPolar_Block.h"

#include <complex>
#include <vector>

CxToPolar_Block::CxToPolar_Block(const std::string& name)
    : Block(name)
{
}

bool CxToPolar_Block::Setup()
{
    Block::Setup();
    return true;
}

bool CxToPolar_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_cxToPolar = std::make_unique<CxToPolar>();

    AddInputPort("input", m_cxToPolar->input, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("magnitude", m_cxToPolar->magnitude, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("phase", m_cxToPolar->phase, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

bool CxToPolar_Block::Run()
{

    if (!m_cxToPolar) {
        return false;
    }

    const std::string inputPortName = GetInputPortName(0);
    auto inputData = ReadInputData<std::complex<double>>(inputPortName);

    if (inputData.empty()) {
        return true;
    }

    std::vector<double> magnitudeData;
    std::vector<double> phaseData;
    magnitudeData.reserve(inputData.size());
    phaseData.reserve(inputData.size());

    for (const auto& x : inputData) {
        magnitudeData.push_back(std::abs(x));
        phaseData.push_back(std::arg(x));
    }

    WriteOutputData(GetOutputPortName(0), magnitudeData);
    WriteOutputData(GetOutputPortName(1), phaseData);

    return true;
}
