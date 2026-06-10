#include "DFErrorHandler.h"

#include <stdexcept>
#include <iostream>

static bool s_errorOccurred = false;

void SystemVueModelBuilder::DFErrorHandler::PostError(DFModel *pModel, const char *pcError)
{
    if(!pcError || !pModel) return;
    s_errorOccurred = true;
    throw std::runtime_error(pcError);
}

void SystemVueModelBuilder::DFErrorHandler::PostWarning(DFModel *pModel, const char *pcWarning)
{
    if(!pModel || !pcWarning) return;
    std::cerr << "[WARNING] " << pcWarning << std::endl;
}

void SystemVueModelBuilder::DFErrorHandler::PostInfo(DFModel *pModel, const char *pcMessage)
{
    if(!pModel || !pcMessage) return;
    std::cout << "[INFO] " << pcMessage << std::endl;
}

void SystemVueModelBuilder::DFErrorHandler::PostLog(DFModel *pModel, const char *pcMessage)
{
    if(!pModel || !pcMessage) return;
    std::cout << "[LOG] " << pcMessage << std::endl;
}

void SystemVueModelBuilder::DFErrorHandler::PostProgress(DFModel *pModel, const char *pcMessage)
{
    if(!pModel || !pcMessage) return;
    std::cout << "[PROGRESS] " << pcMessage << std::endl;
}

void SystemVueModelBuilder::DFErrorHandler::ClearProgress(DFModel *pModel)
{
    if(!pModel) return;
}

bool SystemVueModelBuilder::DFErrorHandler::StopRequested()
{
    return false;
}

bool SystemVueModelBuilder::DFErrorHandler::ErrorOccurred()
{
    return s_errorOccurred;
}


