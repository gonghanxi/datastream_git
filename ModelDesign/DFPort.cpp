#include "DFPort.h"
#include "CDFPortImplementation.h"
#include <QDebug>


SystemVueModelBuilder::DFPort::DFPort()
    : m_cPortImplementation(nullptr)
{
    //初始化
}
SystemVueModelBuilder::DFPort::DFPort(CDFPortImplementation *cPortImplementation)
    : m_cPortImplementation(cPortImplementation)
{
    //初始化
    if(m_cPortImplementation == nullptr) {
        m_cPortImplementation = new CDFPortImplementation();
    }
}

SystemVueModelBuilder::DFPort::DFPort(const DFPort &other)
{
    //初始化
    if (other.m_cPortImplementation) {
        m_cPortImplementation = new CDFPortImplementation(*other.m_cPortImplementation);
    } else {
        m_cPortImplementation = nullptr;
    }
    m_configManager = other.m_configManager;
}

SystemVueModelBuilder::DFPort::~DFPort()
{
    delete m_cPortImplementation;
}

void SystemVueModelBuilder::DFPort::SetName(const char *pcName)
{
    //设置端口名称
    if(m_cPortImplementation && pcName) {
        m_cPortImplementation->SetName(pcName);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetPutType(const char *pcPutType)
{
    //设置端口类型
    if(m_cPortImplementation && pcPutType) {
        m_cPortImplementation->SetPutType(pcPutType);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetPosition(const char *pcPosition)
{
    //设置端口位置
    if(m_cPortImplementation && pcPosition) {
        m_cPortImplementation->SetPosition(pcPosition);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetDataType(const char *pcDataType)
{
    //设置端口数据类型
    if(m_cPortImplementation && pcDataType) {
        m_cPortImplementation->SetDataType(pcDataType);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetOptional(bool bIsOptional)
{
    //设置端口可选
    if(m_cPortImplementation) {
        m_cPortImplementation->SetOptional(bIsOptional);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetRateValue(unsigned int rateVariable)
{
    //设置端口速率值
    if(m_cPortImplementation) {
        m_cPortImplementation->SetRateValue(rateVariable);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetDescription(const char *pcDescription)
{
    //设置端口描述
    if(m_cPortImplementation && pcDescription) {
        m_cPortImplementation->SetDescription(pcDescription);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::SetHideCondition(const char *pcHideCondition)
{
    //设置端口隐藏条件
    if(m_cPortImplementation && pcHideCondition) {
        m_cPortImplementation->SetHideCondition(pcHideCondition);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::AddRateVariable(unsigned int &iRate)
{
    //添加端口速率变量
    if(m_cPortImplementation) {
        m_cPortImplementation->AddRateVariable(iRate);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::AddRateVariableCodeGenName(const char *pccVariableName)
{
    //添加端口速率变量代码生成名称
    if(m_cPortImplementation && pccVariableName) {
        m_cPortImplementation->AddRateVariableCodeGenName(pccVariableName);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFPort::PrependCodeGenName(const char *pcCodeGenPath)
{
    //前置码元代码名称
    if(m_cPortImplementation && pcCodeGenPath) {
        m_cPortImplementation->PrependCodeGenName(pcCodeGenPath);
        SaveConfigToManager();
    }
}

SystemVueModelBuilder::CDFPortImplementation *SystemVueModelBuilder::DFPort::GetImplementation() const
{
    //获取实现类指针
    return m_cPortImplementation;
}
void SystemVueModelBuilder::DFPort::SetConfigManager(std::shared_ptr<ConfigManager> manager)
{
    //设置端口配置
    m_configManager = manager;
    if(m_cPortImplementation) {
        m_cPortImplementation->SetConfigManager(manager);
    }
}
void SystemVueModelBuilder::DFPort::SaveConfigToManager()
{
    //保存配置到管理器
    if(m_cPortImplementation && m_configManager) {
        try {
            PortConfig portConfig;
            portConfig.name = m_cPortImplementation->GetName();
            portConfig.position = m_cPortImplementation->GetPosition();
            portConfig.dataType = m_cPortImplementation->GetDataType();
            portConfig.putType = m_cPortImplementation->GetPutType();
            portConfig.description = m_cPortImplementation->GetDescription();
            portConfig.hideCondition = m_cPortImplementation->GetHideCondition();
            portConfig.codeGenPath = m_cPortImplementation->GetCodeGenPath();
            portConfig.rateVariableNames = m_cPortImplementation->GetRateVariableCodeGenNames();

            portConfig.rateValue = m_cPortImplementation->GetRateValue();
            // auto rateVar = m_cPortImplementation->GetRateVariable();
            // if (rateVar != nullptr) {
            //     portConfig.rateValue = *rateVar;
            // } else {
            //     portConfig.rateValue = 0;
            // }

            portConfig.isOptional = m_cPortImplementation->IsOptional();

            m_configManager->SavePortConfig(m_cPortImplementation->GetName(), portConfig);

        } catch (const std::exception& e) {
            qDebug() << "Failed to save parameter config:" << e.what();
        }
    } else {
        qDebug() << "SaveConfigToManager: m_cPortImplementation =" << m_cPortImplementation
                 << "m_configManager =" << m_configManager.get();
    }
}




