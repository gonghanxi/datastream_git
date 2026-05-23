#ifdef _MSC_VER
    #pragma warning(disable:4819)
#endif
#include "CDFPortImplementation.h"
#include <algorithm>


SystemVueModelBuilder::CDFPortImplementation::CDFPortImplementation()
    : m_isOptional(false)
    , m_rateVariable(nullptr)
    , m_rateValue(0)
    , m_configKey(" ")
{
    //初始化
}

SystemVueModelBuilder::CDFPortImplementation::CDFPortImplementation(const std::string &configKey)
    : m_isOptional(false)
    , m_rateVariable(nullptr)
    , m_rateValue(0)
    , m_configKey(configKey)
{
    //初始化
    LoadFromConfig();
}

SystemVueModelBuilder::CDFPortImplementation::CDFPortImplementation(const DFPort &dfPort)
    : m_isOptional(false)
    , m_rateVariable(nullptr)
    , m_rateValue(0)
    , m_configKey(" ")
{
    // 从DFPort对象初始化
    if (dfPort.GetImplementation()) {
        CDFPortImplementation* impl = dfPort.GetImplementation();
        m_name = impl->m_name;
        m_position = impl->m_position;
        m_putType = impl->m_putType;
        m_description = impl->m_description;
        m_hideCondition = impl->m_hideCondition;
        m_codeGenPath = impl->m_codeGenPath;
        m_rateVariableNames = impl->m_rateVariableNames;
        m_isOptional = impl->m_isOptional;
        m_rateVariable = impl->m_rateVariable;
    }
}

bool SystemVueModelBuilder::CDFPortImplementation::HasConfigManager() const
{
    return m_configManager != nullptr;
}

void SystemVueModelBuilder::CDFPortImplementation::SetConfigManager(std::shared_ptr<ConfigManager> manager)
{
    //设置配置
    m_configManager = manager;
}

std::shared_ptr<SystemVueModelBuilder::ConfigManager> SystemVueModelBuilder::CDFPortImplementation::GetConfigManager() const
{
    return m_configManager;
}

void SystemVueModelBuilder::CDFPortImplementation::AutoSaveConfig()
{
    //自动保存配置
    if (m_configManager && !m_name.empty()) {
        PortConfig config = ToPortConfig();
        m_configManager->SavePortConfig(m_name, config);
    }
}

void SystemVueModelBuilder::CDFPortImplementation::SetConfigKey(const std::string &configKey)
{
    //设置配置键
    m_configKey = configKey;
}

const std::string &SystemVueModelBuilder::CDFPortImplementation::GetConfigKey() const
{
    return m_configKey;
}

bool SystemVueModelBuilder::CDFPortImplementation::SaveToConfig(ConfigManager &externalManager)
{
    //设置保存到配置
    if(m_configKey.empty()) {
        return false;
    }

    PortConfig config = ToPortConfig();
    externalManager.SavePortConfig(m_configKey,config);
    return true;
}

bool SystemVueModelBuilder::CDFPortImplementation::LoadFromConfig()
{
    //加载配置
    if(m_configKey.empty()) {
        return false;
    }
    if(m_configManager->HasPortConfig(m_configKey)) {
        PortConfig config = m_configManager->LoadPortConfig(m_configKey);
        FromPortConfig(config);
        return true;
    }
    return false;
}

bool SystemVueModelBuilder::CDFPortImplementation::RemoveFromConfig()
{
    //移除配置
    if(m_configKey.empty()) {
        return false;
    }
    m_configManager->RemovePortConfig(m_configKey);
    return true;
}

bool SystemVueModelBuilder::CDFPortImplementation::HasConfig() const
{
    //是否有配置
    return (!m_configKey.empty()) &&  (m_configManager->HasPortConfig(m_configKey));
}

void SystemVueModelBuilder::CDFPortImplementation::FromPortConfig(const PortConfig &config)
{
    //读取配置
    m_name = config.name;
    m_position = config.position;
    m_putType = config.putType;
    m_description = config.description;
    m_hideCondition = config.hideCondition;
    m_codeGenPath = config.codeGenPath;
    m_rateVariableNames = config.rateVariableNames;
    m_isOptional = config.isOptional;

    //m_rateVariable
}

SystemVueModelBuilder::PortConfig SystemVueModelBuilder::CDFPortImplementation::ToPortConfig() const
{
    //写入配置
    PortConfig config;
    config.name = m_name;
    config.position = m_position;
    config.putType = m_putType;
    config.description = m_description;
    config.hideCondition = m_hideCondition;
    config.codeGenPath = m_codeGenPath;
    config.rateVariableNames = m_rateVariableNames;
    config.isOptional = m_isOptional;

    if(m_rateVariable != nullptr) {
        config.rateValue = *m_rateVariable;
    }
    return config;
}

void SystemVueModelBuilder::CDFPortImplementation::SetDataType(const char *pcDataType)
{
    //设置端口数据类型
    if(pcDataType)
        m_dataType = pcDataType;
}

const char *SystemVueModelBuilder::CDFPortImplementation::GetDataType() const
{
    return m_dataType.c_str();
}

void SystemVueModelBuilder::CDFPortImplementation::SetPutType(const char *pcPutType)
{
    //设置端口类型
    if(pcPutType)
        m_putType = pcPutType;
}

const char *SystemVueModelBuilder::CDFPortImplementation::GetPutType() const
{
    return m_putType.c_str();
}

void SystemVueModelBuilder::CDFPortImplementation::SetPosition(const char *pcPosition)
{
    //设置端口位置
    if(pcPosition)
        m_position = pcPosition;
}

const char *SystemVueModelBuilder::CDFPortImplementation::GetPosition() const
{
    return m_position.c_str();
}

void SystemVueModelBuilder::CDFPortImplementation::SetName(const char *pcName)
{
    //设置端口名称
    if(pcName) {
        m_name = pcName;
    }
}

const char *SystemVueModelBuilder::CDFPortImplementation::GetName() const
{
    return m_name.c_str();
}

void SystemVueModelBuilder::CDFPortImplementation::SetOptional(bool bIsOptional)
{
    //设置端口是否可选
    m_isOptional = bIsOptional;
}

bool SystemVueModelBuilder::CDFPortImplementation::IsOptional() const
{
    return m_isOptional;
}

unsigned int *SystemVueModelBuilder::CDFPortImplementation::GetRateVariable()
{
    //设置端口速率变量
    return m_rateVariable;
}

void SystemVueModelBuilder::CDFPortImplementation::SetDescription(const char *pcDescription)
{
    //设置端口描述
    if(pcDescription) {
        m_description = pcDescription;
    }
}

const char *SystemVueModelBuilder::CDFPortImplementation::GetDescription()
{
    return m_description.c_str();
}

void SystemVueModelBuilder::CDFPortImplementation::SetHideCondition(const char *pcHideCondition)
{
    //设置端口隐藏条件
    if(pcHideCondition) {
        m_hideCondition = pcHideCondition;
    }
}

void SystemVueModelBuilder::CDFPortImplementation::SetRateValue(unsigned int rateVariable)
{

    //设置端口速率值
    m_rateValue = rateVariable;
}

const char *SystemVueModelBuilder::CDFPortImplementation::GetHideCondition()
{
    return m_hideCondition.c_str();
}

unsigned int SystemVueModelBuilder::CDFPortImplementation::GetRateValue() const
{
    return m_rateValue;
}

const std::string &SystemVueModelBuilder::CDFPortImplementation::GetCodeGenPath() const
{
    //获取代码生成路径
    return m_codeGenPath;
}

void SystemVueModelBuilder::CDFPortImplementation::AddRateVariable(unsigned int &iRate)
{
    //添加速率变量
    m_rateVariable = &iRate;
}

void SystemVueModelBuilder::CDFPortImplementation::AddRateVariableCodeGenName(const char *pccVariableName)
{
    //添加速率变量代码生成名称
    if(pccVariableName) {
        m_rateVariableNames.push_back(pccVariableName);
    }
}



const std::vector<std::string> &SystemVueModelBuilder::CDFPortImplementation::GetRateVariableCodeGenNames()
{
    return m_rateVariableNames;
}


void SystemVueModelBuilder::CDFPortImplementation::PrependCodeGenName(const char *pcCodeGenPath)
{
    //前置码元名称
    if(pcCodeGenPath) {
        if(m_codeGenPath.empty()) {
            m_codeGenPath = pcCodeGenPath;
        }
        else {
            m_codeGenPath = std::string(pcCodeGenPath) + "." + m_codeGenPath;
        }
    }
}

std::string SystemVueModelBuilder::CDFPortImplementation::GetFullCodeGenName() const
{
    //获取完整代码生成名称
    if(m_codeGenPath.empty()) {
        return m_name;
    }
    else {
        return m_codeGenPath + "." + m_name;
    }
}

bool SystemVueModelBuilder::CDFPortImplementation::Validate() const
{
    //判断端口是否有效
    if(m_name.empty()) {
        return false;
    }
    if(!m_hideCondition.empty()) {
        if(m_hideCondition.find("~=") == std::string::npos &&
                m_hideCondition.find("==") == std::string::npos &&
                m_hideCondition.find("!=") == std::string::npos &&
                m_hideCondition.find('>') == std::string::npos &&
                m_hideCondition.find('<') == std::string::npos)
        {
            return false;
        }
    }
    return true;
}

void SystemVueModelBuilder::CDFPortImplementation::Reset()
{
    //重置配置
    m_name.clear();
    m_description.clear();
    m_hideCondition.clear();
    m_codeGenPath.clear();
    m_rateVariableNames.clear();
    m_isOptional = false;
    m_rateVariable = nullptr;
}

