#ifndef CXTOENV_BLOCK_H
#define CXTOENV_BLOCK_H

#include "Block.h"
#include "CxToEnv.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CxToEnv_Block : public SystemVueModelBuilder::Block
{
public:
    //========适配步骤一========/
    CxToEnv_Block(const std::string& name);
    ~CxToEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    //========适配步骤一========/



    void SetParameter(double fc = 0.2e6);


    //========适配步骤十一========/
    double GetCharacterizationFrequency() const;
private:
    //========适配步骤二========/
    void SetDefaultParameters();
    //========适配步骤二========/

    void UpdateCharacterizationFrequency();
    //========适配步骤十一========/

    //========适配步骤七========/
    void ProcessComplexToEnvelope();
    //========适配步骤七========/

    //========适配步骤三========/
    std::unique_ptr<CxToEnv> m_cxToEnv;
    double m_fc;
    //========适配步骤三========/


};

//========适配步骤十========/
RegAlgo(CxToEnv_Block);
//========适配步骤十========/

#endif // CXTOENV_BLOCK_H
