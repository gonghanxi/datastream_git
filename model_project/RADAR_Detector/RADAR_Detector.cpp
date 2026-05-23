#include "RADAR_Detector.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Detector)
{
    SET_MODEL_DESCRIPTION("RADAR Detector");
    SET_MODEL_CATEGORY("Signal Processing");

    ADD_MODEL_INPUT(input);
    ADD_MODEL_OUTPUT(output);

    {
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(DetectorType, SelectedDetectorType);
        enumParam.AddEnumeration("Envelop", Envelop);
        enumParam.AddEnumeration("Square", Square);
        enumParam.AddEnumeration("LogSquare", LogSquare);
        enumParam.AddEnumeration("Log", Log);
        enumParam.SetDefaultValue("1");
    }
    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Log_Coefb);
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("1");
        param.SetHideCondition("DetectorType == 0 || DetectorType == 1");
    }
    {
        SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Log_Coefa);
        param.SetUnit(SystemVueModelBuilder::Units::NONE);
        param.SetDefaultValue("1");
        param.SetHideCondition("DetectorType == 0 || DetectorType == 1");
    }
    return true;
}
#endif

RADAR_Detector::RADAR_Detector()
    : DetectorType(Square)
    , Log_Coefb(1.0)
    , Log_Coefa(1.0)
{
}

bool RADAR_Detector::Run()
{
    const double absx = std::abs(input[0]);

    switch (DetectorType)
    {
    case RADAR_Detector::Envelop:
        output[0] = absx;
        break;
    case RADAR_Detector::Square:
        output[0] = absx * absx;
        break;
    case RADAR_Detector::LogSquare:
        output[0] = Log_Coefa * std::log(std::abs(Log_Coefb * input[0]) * std::abs(Log_Coefb * input[0]));
        break;
    case RADAR_Detector::Log:
        output[0] = Log_Coefa * std::log(std::abs(Log_Coefb * input[0]));
        break;
    default:
        break;
    }
    return true;
}
