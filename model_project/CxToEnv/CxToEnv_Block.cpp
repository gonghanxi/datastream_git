#include "CxToEnv_Block.h"

//========适配步骤四========/
//template class CXTOENV_API std::map<std::string, Parameter>;
//template class CXTOENV_API std::map<int, PortMsg>;
//========适配步骤四========/

//========适配步骤四========/
CxToEnv_Block::CxToEnv_Block(const std::string &name)
    :Block(name)
{
}
//========适配步骤四========/





//========适配步骤六========/
bool CxToEnv_Block::Setup()
{
    Block::Setup();
    if(m_cxToEnv) {
    }
    return true;
}
//========适配步骤六========/

//========适配步骤七========/
bool CxToEnv_Block::Run()
{
    UpdateCharacterizationFrequency();

    ProcessComplexToEnvelope();

    //ProcessComplexToEnvelopeReal();

    //ProcessComplexToEnvelopeNewFc();

    return true;
}
//========适配步骤七========/



//========适配步骤八========/
bool CxToEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_cxToEnv = std::make_unique<CxToEnv>();

    AddInputPort("Cx", m_cxToEnv->Cx, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("Fc", m_cxToEnv->Fc, 1, DataType::ENVELOPE_SIGNAL);
    AddOutputPort("Env", m_cxToEnv->Env, 1, DataType::ENVELOPE_SIGNAL);
    //========适配步骤八========/

    //========适配步骤九========/
    SetDefaultParameters();

    m_fc = std::stod(getParameter("Fc").Value);
    //========适配步骤九========/

    SetParameter(m_fc);

    return true;
}

//========适配步骤九========/
void CxToEnv_Block::SetDefaultParameters()
{
    m_fc = 0.2e6;
}

void CxToEnv_Block::SetParameter(double fc)
{
    m_fc = fc;
    if(m_cxToEnv) {
        m_cxToEnv->fc = m_fc;
    }
}
//========适配步骤九========/

//========适配步骤八========/
void CxToEnv_Block::ProcessComplexToEnvelope()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    //----------------读取数据---------------------
    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if(inputData.empty()) {
        return;
    }

    //----------------数据处理---------------------
    std::vector<SystemVueModelBuilder::EnvelopeSignal> envelopeData;
    envelopeData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        const auto& cxSample = inputData[i];

        SystemVueModelBuilder::EnvelopeSignal envSignal(cxSample);
        envelopeData.push_back(envSignal);
    }
    //----------------数据处理---------------------

    //----------------写入数据---------------------
    WriteOutputData(outputPortName, envelopeData);
    Buffer* outputBuffer = GetOutputPort(outputPortName);
    BufferReader* fcReader = GetInputPort(GetInputPortName(1));
    if(fcReader->IsConnected()) {
        auto fcData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(GetInputPortName(1));
        outputBuffer->setCharacterizationFrequency(fcData[0].real());
    }
    else {
        outputBuffer->setCharacterizationFrequency(m_fc);
    }


}
//========适配步骤八========/



//========适配步骤十一========/
double CxToEnv_Block::GetCharacterizationFrequency() const
{
    return m_fc;
}

void CxToEnv_Block::UpdateCharacterizationFrequency()
{
    BufferReader* fcReader = GetInputPort(GetInputPortName(1));
    if(fcReader->IsConnected()) {
        if(m_cxToEnv) {
            m_cxToEnv->Fc.SetConnected(true);
            double fc = fcReader->getCharacterizationFrequency();
            Buffer* outputBuffer = GetOutputPort("Env");
            if(outputBuffer) {
                outputBuffer->setCharacterizationFrequency(fc);
                m_fc = fc;
            }
        }
    }
}
//========适配步骤十一========/
