#ifdef _MSC_VER
    #pragma warning(disable:4819)
#endif
#include "CDFParamImplementation.h"
#include <algorithm>
#include <stdexcept>
#include "configmanager.h"



    //ConfigManager CDFParamImplementation::m_configManager;

    SystemVueModelBuilder::CDFParamImplementation::CDFParamImplementation()
        :m_unitType(Units::NONE),
        m_isFileParam(false),
        m_isSchematicDisplay(false),
        m_isNonSetable(false),
        m_isDynamicUpdateSupported(false),
        m_useDefault(false),
        m_configKey("")
    {
        //初始化
        m_configManager = std::make_shared<ConfigManager>();
    }

    SystemVueModelBuilder::CDFParamImplementation::CDFParamImplementation(const std::string &configKey)
        : m_unitType(Units::NONE),
        m_isFileParam(false),
        m_isSchematicDisplay(false),
        m_isNonSetable(false),
        m_isDynamicUpdateSupported(false),
        m_useDefault(false),
        m_configKey(configKey)
    {
        //初始化
        LoadFromConfig();
    }

    SystemVueModelBuilder::CDFParamImplementation::CDFParamImplementation(const DFParam &dfParam)
        : m_unitType(Units::NONE),
        m_isFileParam(false),
        m_isSchematicDisplay(false),
        m_isNonSetable(false),
        m_isDynamicUpdateSupported(false),
        m_useDefault(false),
        m_configKey("")
    {
        //初始化
        CDFParamImplementation* impl = dfParam.GetImplementation();
        if (impl)
        {
            m_name = impl->GetName();
            m_description = impl->GetDescription();
            m_defaultValue = impl->GetDefaultValue();
            m_unitType = impl->GetUnit();
            m_isFileParam = impl->IsFileParam();
            m_paramType = impl->GetParamType();
            m_enumerations = impl->GetEnumeration();
            m_codeGenPath = impl->GetCodeGenName();
            m_hideCondition = impl->GetHideCondition();
            m_isSchematicDisplay = impl->IsSchematicDisplay();
            m_isNonSetable = impl->isNonSetable();
            m_isDynamicUpdateSupported = impl->isDynamicUpdateSupported();
            m_useDefault = impl->UseDefault();
            m_configKey = impl->GetConfigKey();
        }
    }

    bool SystemVueModelBuilder::CDFParamImplementation::HasConfigManager() const
    {
        //判断配置是否存在
        return m_configManager != nullptr;
    }

    void SystemVueModelBuilder::CDFParamImplementation::SetConfigManager(std::shared_ptr<ConfigManager> manager)
    {
        //设置配置
        m_configManager = manager;
    }

    std::shared_ptr<SystemVueModelBuilder::ConfigManager> SystemVueModelBuilder::CDFParamImplementation::GetConfigManager() const
    {
        //获取配置指针
        return m_configManager;
    }

    void SystemVueModelBuilder::CDFParamImplementation::AutoSaveConfig()
    {
        //自动保存配置
        if (m_configManager && !m_name.empty()) {
            ParamConfig config = ToParamConfig();
            m_configManager->SaveParamConfig(m_name, config);
        }
    }

    void SystemVueModelBuilder::CDFParamImplementation::SetConfigKey(const std::string &configKey)
    {
        //设置配置键
        m_configKey = configKey;
    }
    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetConfigKey() const
    {
        //获取配置键
        return m_configKey;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::SaveToConfig(ConfigManager &externalManager)
    {
        //保存配置
        if(m_configKey.empty()) {
            return false;
        }
        ParamConfig config = ToParamConfig();
        externalManager.SaveParamConfig(m_configKey, config);
        return true;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::LoadFromConfig()
    {
        //加载配置
        if(m_configKey.empty()) {
            return false;
        }
        if(m_configManager->HasParamConfig(m_configKey)) {
            ParamConfig config = m_configManager->LoadParamConfig(m_configKey);
            FromParamConfig(config);
            return true;
        }
        return false;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::RemoveFromConfig()
    {
        //移除配置
        if(m_configKey.empty()) {
            return false;
        }
        m_configManager->RemoveParamConfig(m_configKey);
        return true;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::HasConfig() const
    {
        //判断配置是否存在
        return !m_configKey.empty() && m_configManager->HasParamConfig(m_configKey);
    }

    void SystemVueModelBuilder::CDFParamImplementation::FromParamConfig(const ParamConfig &config)
    {
        //读取配置
        m_name = config.name;
        m_description = config.description;
        m_paramType = config.dataType;
        m_defaultValue = config.defaultValue;
        m_value = config.Value;
        m_unitType = config.unit;
        //转换枚举值
        m_enumerations.clear();
        for(size_t i = 0; i < config.enumValues.size(); i++) {
            m_enumerations.emplace_back(config.enumValues[i], static_cast<int>(i));
        }

        m_hideCondition = config.hideCondition;
    }

    void SystemVueModelBuilder::CDFParamImplementation::UpdateEnumerationTypeFromEnumerations()
    {
        //更新枚举值
        if (m_enumerations.size() == 2) {
            const auto& first = m_enumerations[0];
            const auto& second = m_enumerations[1];

            if (first.first == "QUERY_NO" && first.second == 0 &&
                second.first == "QUERY_YES" && second.second == 1) {
                m_enumerationType = QUERY_ENUM;
            }
            else if (first.first == "SWITCH_OFF" && first.second == 0 &&
                     second.first == "SWITCH_ON" && second.second == 1) {
                m_enumerationType = SWITCH_ENUM;
            }
            else if (first.first == "BOOLEAN_FALSE" && first.second == 0 &&
                     second.first == "BOOLEAN_TRUE" && second.second == 1) {
                m_enumerationType = BOOLEAN_ENUM;
            }
            else {
                m_enumerationType.clear();
            }
        } else {
            m_enumerationType.clear();
        }
    }

    SystemVueModelBuilder::ParamConfig SystemVueModelBuilder::CDFParamImplementation::ToParamConfig() const
    {
        //写入配置
        ParamConfig config;
        config.name = m_name;
        config.description = m_description;
        config.dataType = m_paramType;
        config.defaultValue = m_defaultValue;
        config.Value = m_value;
        config.unit = m_unitType;

        // 转换枚举值
        config.enumValues.clear();
        for (const auto& enumPair : m_enumerations) {
            config.enumValues.push_back(enumPair.first);
        }

        config.hideCondition = m_hideCondition;
        config.isAdvanced = m_isNonSetable;

        return config;
    }




    //
    void SystemVueModelBuilder::CDFParamImplementation::SetName(const char *pcName)
    {
        //设置参数名称
        if(pcName) {
            m_name = pcName;
        }

    }
    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetName() const
    {
        return m_name;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetDescription(const char *pcDescription)
    {
        //设置参数描述
        if(pcDescription) {
            m_description = pcDescription;
        }
        if(m_configManager) {
            ParamConfig paramConfig = ToParamConfig();
            m_configManager->SaveParamConfig(m_configKey, paramConfig);
        }
    }

    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetDescription() const
    {
        return m_description;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetDefaultValue(const char *pcValue)
    {
        //设置参数默认值
        if(pcValue) {
            m_value = m_defaultValue = pcValue;
        }
        if(m_configManager) {
            ParamConfig paramConfig = ToParamConfig();
            m_configManager->SaveParamConfig(m_configKey, paramConfig);
        }
    }



    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetDefaultValue() const
    {
        return m_defaultValue;
    }


    //
    void SystemVueModelBuilder::CDFParamImplementation::SetUnit(Units::UnitType eUnitType)
    {
        //设置参数单位类型
        m_unitType = eUnitType;
        if(m_configManager) {
            ParamConfig paramConfig = ToParamConfig();
            m_configManager->SaveParamConfig(m_configKey, paramConfig);
        }
    }

    SystemVueModelBuilder::Units::UnitType SystemVueModelBuilder::CDFParamImplementation::GetUnit() const
    {
        return m_unitType;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetParamAsFile()
    {
        //设置参数文件
        if( ( !m_paramType.empty() ) && ( m_paramType != "string" ) && ( m_paramType != "std::string" ) ) {
            throw std::runtime_error("SetParamAsFile() is only valid for string type parameters");
        }
        m_isFileParam = true;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::IsFileParam() const
    {
        //是否为文件参数
        return m_isFileParam;
    }

    void SystemVueModelBuilder::CDFParamImplementation::SetParamType(const char *pcType)
    {
        //设置参数类型
        if(pcType) {
            m_paramType = pcType;
        }
        if(m_isFileParam && ( m_paramType != "string" ) && ( m_paramType != "std::string" )) {
            m_isFileParam = false;
        }
    }

    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetParamType() const
    {
        return m_paramType;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::AddEnumeration(const char *pcEnumName, int iEnumValue)
    {
        //添加枚举参数
        if(pcEnumName) {
            m_enumerations.emplace_back(pcEnumName,iEnumValue);
        }
        if(m_configManager) {
            ParamConfig paramConfig = ToParamConfig();
            m_configManager->SaveParamConfig(m_name, paramConfig);
        }
    }

    const std::vector<std::pair<std::string, int> > &SystemVueModelBuilder::CDFParamImplementation::GetEnumeration() const
    {
        return m_enumerations;
    }

    void SystemVueModelBuilder::CDFParamImplementation::SetEnumeration(const char *EnumerationName)
    {
        //设置枚举参数
        if(!EnumerationName) {
            m_enumerationType.clear();
            return;
        }
        m_enumerations.clear();
        m_enumerationType = EnumerationName;

        std::string enumName(EnumerationName);

        if (enumName == QUERY_ENUM) {
            m_enumerations.emplace_back("QUERY_NO", 0);
            m_enumerations.emplace_back("QUERY_YES", 1);
        }
        else if(enumName == SWITCH_ENUM) {
            m_enumerations.emplace_back("SWITCH_OFF", 0);
            m_enumerations.emplace_back("SWITCH_ON", 1);
        }
        else if(enumName == BOOLEAN_ENUM) {
            m_enumerations.emplace_back("BOOLEAN_FALSE", 0);
            m_enumerations.emplace_back("BOOLEAN_TRUE", 1);
        }
        else {
            m_enumerationType.clear();
            throw std::runtime_error("Invalid predefined enumeration name: " + enumName);
        }
        // 确保该函数只对整数参数有效
        if (m_paramType.empty() || (m_paramType != "int" && m_paramType != "integer")) {
            m_paramType = "int";
        }
    }

    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetEnumerationType() const
    {
        return m_enumerationType;
    }

    void SystemVueModelBuilder::CDFParamImplementation::SetSizeParameterName(const char *pcSizeName)
    {
        //设置大小参数名称
        if(pcSizeName)
            m_sizeParameterName = pcSizeName;
    }

    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetSizeParameterName() const
    {
        return m_sizeParameterName;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::IsArrayParameter() const
    {
        //判断是否是数组参数
        return !m_sizeParameterName.empty() ||
               (m_paramType.find("[]") != std::string::npos);
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::PrependCodeGenName(const char *pcCodeGenPath)
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

    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetCodeGenName() const
    {
        return m_codeGenPath;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetHideCondition(const char *pcHideConditon)
    {
        //设置参数隐藏条件
        if(pcHideConditon) {
            m_hideCondition = pcHideConditon;
        }
        if(m_configManager) {
            ParamConfig paramConfig = ToParamConfig();
            m_configManager->SaveParamConfig(m_configKey, paramConfig);
        }
    }

    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetHideCondition() const
    {
        return m_hideCondition;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetSchematicDisplay(bool bDisplay)
    {
        //设置视图显示
        m_isSchematicDisplay = bDisplay;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::IsSchematicDisplay() const
    {
        return m_isSchematicDisplay;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetNonSetable(bool bNonSetable)
    {
        //设置禁止修改参数
        m_isNonSetable = bNonSetable;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::isNonSetable() const
    {
        return m_isNonSetable;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetDynamicUpdate(bool bDynamicUpdateSupported)
    {
        //设置参数动态更新
        m_isDynamicUpdateSupported = bDynamicUpdateSupported;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::isDynamicUpdateSupported() const
    {
        return m_isDynamicUpdateSupported;
    }
    //
    void SystemVueModelBuilder::CDFParamImplementation::SetUseDefault(bool bFlag)
    {
        //设置参数用户默认
        m_useDefault = bFlag;
    }

    bool SystemVueModelBuilder::CDFParamImplementation::UseDefault() const
    {
        return m_useDefault;
    }
    //
    bool SystemVueModelBuilder::CDFParamImplementation::Validate() const
    {
        //判断参数有效
        if(m_name.empty()) {
            return false;
        }
        if(!m_hideCondition.empty()) {
            if (m_hideCondition.find("~=") == std::string::npos &&
                m_hideCondition.find("==") == std::string::npos &&
                m_hideCondition.find("!=") == std::string::npos &&
                m_hideCondition.find('>') == std::string::npos &&
                m_hideCondition.find('<') == std::string::npos)
            {
                return false;
            }
        }
        if(m_isFileParam) {
            if( ( m_paramType != "string" ) && ( m_paramType != "std::string" ) && !m_paramType.empty() ) {
                return false;
            }
        }
        return true;
    }

    void SystemVueModelBuilder::CDFParamImplementation::Reset()
    {
        //重置所有参数变量
        m_name.clear();
        m_description.clear();
        m_defaultValue.clear();
        m_hideCondition.clear();
        m_codeGenPath.clear();
        m_enumerations.clear();
        m_enumerationType.clear();
        m_unitType = Units::NONE;
        m_isFileParam = false;
        m_isSchematicDisplay = false;
        m_isNonSetable = false;
        m_isDynamicUpdateSupported = false;
        m_useDefault = false;
    }

    void SystemVueModelBuilder::CDFParamImplementation::SetValue(const char *pcValue)
    {
        //设置参数值
        if(pcValue) {
            m_value = pcValue;
        }
        if(m_configManager) {
            ParamConfig paramConfig = ToParamConfig();
            m_configManager->SaveParamConfig(m_configKey, paramConfig);
        }
    }
    const std::string &SystemVueModelBuilder::CDFParamImplementation::GetValue() const
    {
        return m_value;
    }


