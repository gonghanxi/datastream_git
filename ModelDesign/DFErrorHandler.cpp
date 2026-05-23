#include "DFErrorHandler.h"

#include <stdexcept>

class ErrorHandlerState
{
public:
};

void SystemVueModelBuilder::DFErrorHandler::PostError(DFModel *pModel, const char *pcError)
{
    //记录错误
    if(!pcError || !pModel) return;

    throw std::runtime_error(pcError);
}

void SystemVueModelBuilder::DFErrorHandler::PostWarning(DFModel *pModel, const char *pcWarning)
{
    //记录警告
    if(!pModel || !pcWarning) return;
}

void SystemVueModelBuilder::DFErrorHandler::PostInfo(DFModel *pModel, const char *pcMessage)
{
    //记录输入
    if(!pModel || !pcMessage) return;
}

void SystemVueModelBuilder::DFErrorHandler::PostLog(DFModel *pModel, const char *pcMessage)
{
    //记录日志
    if(!pModel || !pcMessage) return;
}

void SystemVueModelBuilder::DFErrorHandler::PostProgress(DFModel *pModel, const char *pcMessage)
{
    //记录处理
    if(!pModel || !pcMessage) return;
}

void SystemVueModelBuilder::DFErrorHandler::ClearProgress(DFModel *pModel)
{
    //清空处理
    if(!pModel) return;
}

bool SystemVueModelBuilder::DFErrorHandler::StopRequested()
{
    return false;
}

bool SystemVueModelBuilder::DFErrorHandler::ErrorOccurred()
{
    return true;
}


