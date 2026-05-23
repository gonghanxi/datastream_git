#include "CxToEnv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(CxToEnv)
{
    SET_MODEL_DESCRIPTION("Convert complex signal to envelope signal");
    SET_MODEL_SYMBOL("SYM_CxToEnv");
    SET_MODEL_CATEGORY("Analog/RF");
    SET_MODEL_CATEGORY("Type Converters");


    ADD_MODEL_INPUT(Cx);

    {
        SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(Fc);
        port.SetOptional();
    }


    ADD_MODEL_OUTPUT(Env);


    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(fc);
        param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
        param.SetDefaultValue("0.2e6");
    }

    return true;
}
#endif

CxToEnv::CxToEnv()
{
}

//-----------------------------------------------------------------------------------
//	Characterization frequency propagate
//		Set the characterization frequency from input port Fc (if input).
//-----------------------------------------------------------------------------------

ERESULT CxToEnv::PropagateCharacterizationFrequency()
{
    bool bStatus = true;

    if (Fc.IsConnected())
    {
        fc = Fc.GetCharacterizationFrequency();
    }

    if (fc > 0)
    {
        Env.SetCharacterizationFrequency(fc);
    }
    else
    {
        POST_ERROR("characterization frequency must be greater than 0.");
        bStatus = false;
    }
    return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CxToEnv::Run()
{
    //Env[0] = SystemVueModelBuilder::EnvelopeSignal( Cx );
    Env[0] = Cx[0];

    return true;
}
