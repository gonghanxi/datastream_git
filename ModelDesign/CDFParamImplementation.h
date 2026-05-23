#ifndef CDFPARAMIMPLEMENTATION_H
#define CDFPARAMIMPLEMENTATION_H

#include <string>
#include <vector>
#include "DFEnumerations.h"
#include "DFParam.h"
#include "configmanager.h"


namespace SystemVueModelBuilder {
    class CDFParamImplementation
    {
    private:
        //参数配置的存储变量
        std::string m_name;
        std::string m_description;
        std::string m_defaultValue; //默认值
        std::string m_value;
        Units::UnitType m_unitType; //单位类型

        bool m_isFileParam;
        std::string m_paramType;
        //枚举
        std::vector<std::pair<std::string, int>> m_enumerations;
        std::string m_enumerationType;

        //数组
        std::string m_sizeParameterName;  //存储数组大小参数名称

        std::string m_codeGenPath; //代码生成路径

        std::string m_hideCondition; //隐藏条件
        bool m_isSchematicDisplay; //是否视图显示
        bool m_isNonSetable; //是否禁止设置
        bool m_isDynamicUpdateSupported; //是否支持动态更新
        bool m_useDefault; //是否使用默认值

        // 配置管理器
        // ConfigManager m_configManager;
        std::shared_ptr<ConfigManager> m_configManager;
        std::string m_configKey;


    public:
        //DFParam的实现类
        CDFParamImplementation();
        explicit CDFParamImplementation(const std::string& configKey);
        explicit CDFParamImplementation(const DFParam& dfParam);
        ~CDFParamImplementation() = default;

        //保存到配置管理器
        bool HasConfigManager() const;
        void SetConfigManager(std::shared_ptr<ConfigManager> manager);
        std::shared_ptr<ConfigManager> GetConfigManager() const;
        void AutoSaveConfig();//自动保存

        // 配置管理功能
        bool SaveToConfig(ConfigManager& externalManager);//保存配置
        bool LoadFromConfig();//加载配置
        bool RemoveFromConfig();//删除配置
        bool HasConfig() const;//是否是配置
        // 从ParamConfig转换
        void FromParamConfig(const ParamConfig& config);
        void UpdateEnumerationTypeFromEnumerations();
        // 转换为ParamConfig
        ParamConfig ToParamConfig() const;

        //设置
        void SetConfigKey(const std::string& configKey);//配置键
        void SetName(const char* pcName);//名称
        void SetDescription(const char* pcDescription);//描述
        void SetDefaultValue(const char* pcValue);//默认值
        void SetValue(const char* pcValue);//数值
        void SetUnit(Units::UnitType eUnitType);//单位
        void SetParamAsFile();//文件参数
        void SetParamType(const char* pcType);//参数类型
        void AddEnumeration(const char* pcEnumName,int iEnumValue);//枚举值
        void SetEnumeration(const char* EnumerationName);//预定义枚举
        void SetSizeParameterName(const char* pcSizeName);//数组大小名称
        void PrependCodeGenName(const char* pcCodeGenPath);//代码生成路径
        void SetHideCondition(const char* pcHideConditon);//隐藏条件
        void SetSchematicDisplay(bool bDisplay);//原理图显示
        void SetNonSetable(bool bNonSetable);//是否可设置
        void SetDynamicUpdate(bool bDynamicUpdateSupported);//动态更新支持
        void SetUseDefault(bool bFlag);//是否使用默认值
        //获取
        const std::string& GetConfigKey() const;
        const std::string& GetName() const;
        const std::string& GetDescription() const;
        const std::string& GetDefaultValue() const;
        const std::string& GetValue() const;
        Units::UnitType GetUnit() const;
        bool IsFileParam() const;//是否为文件参数
        const std::string& GetParamType() const;
        const std::vector<std::pair<std::string, int>>& GetEnumeration() const;
        const std::string& GetEnumerationType() const;
        const std::string& GetSizeParameterName() const;
        bool IsArrayParameter() const;
        const std::string& GetCodeGenName() const;
        const std::string& GetHideCondition() const;
        bool IsSchematicDisplay() const;
        bool isNonSetable() const;
        bool isDynamicUpdateSupported() const;
        bool UseDefault() const;
        //验证参数配置
        bool Validate() const;
        //重置参数配置
        void Reset();
    };

    }
#endif // CDFPARAMIMPLEMENTATION_H
