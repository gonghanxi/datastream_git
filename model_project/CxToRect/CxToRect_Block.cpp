#include "CxToRect_Block.h"

//template class CXTORECT_API std::map<std::string, Parameter>;
//template class CXTORECT_API std::map<int, PortMsg>;

CxToRect_Block::CxToRect_Block(const std::string &name)
    :Block(name)
{
}

bool CxToRect_Block::Setup()
{
    Block::Setup();
    return true;
}

bool CxToRect_Block::Run()
{
    std::string DComplexPort = GetInputPortName(0);
    std::string RealPort = GetOutputPortName(0);
    std::string ImagPort = GetOutputPortName(1);


    //----------------读取数据---------------------
    auto DComplexData = ReadInputData<std::complex<double>>(DComplexPort);

    //----------------数据处理---------------------
    std::vector<double> RealData;
    std::vector<double> ImagData;
    RealData.reserve(DComplexData.size());
    ImagData.reserve(DComplexData.size());
    for(size_t i = 0; i < DComplexData.size(); i++) {
        RealData.push_back(DComplexData[i].real());
        ImagData.push_back(DComplexData[i].imag());
    }
    //----------------数据处理---------------------

    //----------------写入数据---------------------
    WriteOutputData(RealPort, RealData);
    WriteOutputData(ImagPort, ImagData);

    return true;
}

bool CxToRect_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_cxtoRect = std::make_unique<CxToRect>();

    AddInputPort("Cx", m_cxtoRect->Cx, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("Real", m_cxtoRect->Real, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Imag", m_cxtoRect->Imag, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void CxToRect_Block::SetDefaultParamters()
{

}
