#ifndef ADDCX_BLOCK_H
#define ADDCX_BLOCK_H

#include "MATLAB_Script.h"
#include "Block.h"
#include "engine.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MATLAB_Script_Block : public SystemVueModelBuilder::Block
{
public:
    MATLAB_Script_Block(const std::string& name);
    ~MATLAB_Script_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<MATLAB_Script> m_addCx;
private:
     Engine *ep ;
     QString callStr;
};


RegAlgo(MATLAB_Script_Block);
#endif // ADDCX_BLOCK_H
