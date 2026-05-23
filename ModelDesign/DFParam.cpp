#include "DFParam.h"
#include"CDFParamImplementation.h"
#include <QDebug>

SystemVueModelBuilder::DFParam::DFParam(CDFParamImplementation *cParamImplementation)
    : m_cParamImplementation(cParamImplementation)
{
    //初始化
    if(m_cParamImplementation == nullptr) {
        m_cParamImplementation = new CDFParamImplementation();
    }
}

SystemVueModelBuilder::DFParam::DFParam(const DFParam &other)
{
    //初始化
    if (other.m_cParamImplementation) {
        m_cParamImplementation = new CDFParamImplementation(*other.m_cParamImplementation);
    } else {
        m_cParamImplementation = nullptr;
    }
    m_configManager = other.m_configManager;
}
SystemVueModelBuilder::DFParam::DFParam()
    : m_cParamImplementation(nullptr)
{
    //初始化
}
SystemVueModelBuilder::DFParam::~DFParam()
{
    delete m_cParamImplementation;
}

void SystemVueModelBuilder::DFParam::SetName(const char *pcName)
{
    //设置参数名称
    if(m_cParamImplementation && pcName) {
        m_cParamImplementation->SetName(pcName);
    }
}

void SystemVueModelBuilder::DFParam::SetDescription(const char *pcDescription)
{
    //设置参数描述
    if(m_cParamImplementation && pcDescription) {
        m_cParamImplementation->SetDescription(pcDescription);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFParam::SetDefaultValue(const char *pcValue)
{
    //设置参数默认值
    if(m_cParamImplementation && pcValue) {
        m_cParamImplementation->SetDefaultValue(pcValue);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFParam::SetValue(const char *pcValue)
{
    //设置参数值
    if(m_cParamImplementation && pcValue) {
        m_cParamImplementation->SetValue(pcValue);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFParam::SetUnit(Units::UnitType eUnitType)
{
    //设置参数单位
    if(m_cParamImplementation) {
        m_cParamImplementation->SetUnit(eUnitType);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFParam::SetParamAsFile()
{
    //设置文件参数
    if(m_cParamImplementation) {
        m_cParamImplementation->SetParamAsFile();
    }
}

void SystemVueModelBuilder::DFParam::AddEnumeration(const char *pcEnumName, int iEnumValue)
{
    //添加枚举参数
    if(m_cParamImplementation && pcEnumName) {
        m_cParamImplementation->AddEnumeration(pcEnumName,iEnumValue);
    }
}

void SystemVueModelBuilder::DFParam::SetEnumeration(const char *EnumerationName)
{
    //设置枚举参数
    if(m_cParamImplementation && EnumerationName) {
        m_cParamImplementation->SetEnumeration(EnumerationName);
    }
}

void SystemVueModelBuilder::DFParam::PrependCodeGenName(const char *pcCodeGenPath)
{
    //前置码元代码名称
    if(m_cParamImplementation && pcCodeGenPath) {
        m_cParamImplementation->PrependCodeGenName(pcCodeGenPath);
    }
}

void SystemVueModelBuilder::DFParam::SetHideCondition(const char *pcHideCondition)
{
    //设置参数隐藏条件
    if(m_cParamImplementation && pcHideCondition) {
        m_cParamImplementation->SetHideCondition(pcHideCondition);
        SaveConfigToManager();
    }
}

void SystemVueModelBuilder::DFParam::SetSchematicDisplay(bool bDisplay)
{
    //设置参数视图显示
    if(m_cParamImplementation) {
        m_cParamImplementation->SetSchematicDisplay(bDisplay);
    }
}

void SystemVueModelBuilder::DFParam::SetNonSetable(bool bNonSetable)
{
    //设置参数禁止修改
    if(m_cParamImplementation) {
        m_cParamImplementation->SetNonSetable(bNonSetable);
    }
}

void SystemVueModelBuilder::DFParam::SetDynamicUpdate(bool bDynamicUpdateSupported)
{
    //设置参数动态更新
    if(m_cParamImplementation) {
        m_cParamImplementation->SetDynamicUpdate(bDynamicUpdateSupported);
    }
}

void SystemVueModelBuilder::DFParam::SetUseDefault(bool bFlag)
{
    //设置参数用户默认
    if(m_cParamImplementation) {
        m_cParamImplementation->SetUseDefault(bFlag);
    }
}

void SystemVueModelBuilder::DFParam::SetConfigManager(std::shared_ptr<ConfigManager> manager)
{
    //设置参数配置
    m_configManager = manager;
    if(m_cParamImplementation) {
        m_cParamImplementation->SetConfigManager(manager);
    }
}

SystemVueModelBuilder::CDFParamImplementation *SystemVueModelBuilder::DFParam::GetImplementation() const
{
    //获取实现类指针
    return m_cParamImplementation;
}
void SystemVueModelBuilder::DFParam::SaveConfigToManager()
{
    //保存配置到管理器
    if(m_cParamImplementation && m_configManager) {
        try {
            ParamConfig paramConfig;
            paramConfig.name = m_cParamImplementation->GetName();
            paramConfig.dataType = m_cParamImplementation->GetParamType();
            paramConfig.defaultValue = m_cParamImplementation->GetDefaultValue();
            paramConfig.Value = m_cParamImplementation->GetValue();
            paramConfig.description = m_cParamImplementation->GetDescription();
            paramConfig.hideCondition = m_cParamImplementation->GetHideCondition();
            paramConfig.unit = m_cParamImplementation->GetUnit();

            // 保存枚举信息
            auto enumerations = m_cParamImplementation->GetEnumeration();
            paramConfig.enumValues.clear();
            for (const auto& enumPair : enumerations) {
                paramConfig.enumValues.push_back(enumPair.first);
            }
            m_configManager->SaveParamConfig(m_cParamImplementation->GetName(), paramConfig);

        } catch (const std::exception& e) {
            qDebug() << "Failed to save parameter config:" << e.what();
        }
    } else {
        qDebug() << "SaveConfigToManager: m_cParamImplementation =" << m_cParamImplementation
                 << "m_configManager =" << m_configManager.get();
    }
}


