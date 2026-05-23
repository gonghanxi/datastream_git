#ifndef CDFPORTIMPLEMENTATION_H
#define CDFPORTIMPLEMENTATION_H
#ifdef _MSC_VER
    #pragma warning(disable:4819)
#endif

#include <string>
#include <vector>

#include "configmanager.h"
#include "DFPort.h"

namespace SystemVueModelBuilder {
    class CDFPortImplementation
    {
    private:
        //端口配置的存储变量
        std::string m_name;
        std::string m_description;
        std::string m_hideCondition;
        std::string m_codeGenPath;
        std::vector<std::string> m_rateVariableNames; //速率变量名称容器
        std::string m_position; //位置存储
        std::string m_dataType;
        std::string m_putType;
        bool m_isOptional; //是否可选
        unsigned* m_rateVariable; //速率变量
        unsigned int m_rateValue; //速率值


        // 配置管理器
        //static ConfigManager m_PortconfigManager;
        std::shared_ptr<ConfigManager> m_configManager;
        std::string m_configKey;
    public:
        //DFPort的实现类
        CDFPortImplementation();
        explicit CDFPortImplementation(const std::string& configKey);
        explicit CDFPortImplementation(const DFPort& dfPort);
        ~CDFPortImplementation() = default;

        //保存到配置管理器
        bool HasConfigManager() const;
        void SetConfigManager(std::shared_ptr<ConfigManager> manager);
        std::shared_ptr<ConfigManager> GetConfigManager() const;
        void AutoSaveConfig();//自动保存


        // 配置管理功能
        bool SaveToConfig(ConfigManager& externalManager);
        bool LoadFromConfig();
        bool RemoveFromConfig();
        bool HasConfig() const;
        //从PortConfig转换
        void FromPortConfig(const PortConfig& config);
        //转换为PortConfig
        PortConfig ToPortConfig() const;

        //设置
        void SetName(const char* pcName);//名称
        void SetOptional(bool bIsOptional);//是否可选
        void AddRateVariable(unsigned &iRate);//速率变量
        void SetDataType(const char* pcDataType);//参数类型
        void SetPosition(const char* pcPosition);//端口位置
        void SetPutType(const char* pcPutType);//输入输出类型
        void SetDescription(const char* pcDescription);//描述
        void SetConfigKey(const std::string& configKey);//配置键
        void SetHideCondition(const char* pcHideCondition);//隐藏条件
        void SetRateValue(unsigned int rateVariable);//速率变量
        //void SetCodeGenPath(const std::string& codeGenPath);//代码生成名称
        void PrependCodeGenName(const char* pcCodeGenPath);//前置代码生成名称路径
        void AddRateVariableCodeGenName(const char* pccVariableName);//速率变量代码生成名称
        //获取
        const char* GetName() const;//名称
        bool IsOptional() const;//判断是否可选

        unsigned *GetRateVariable();//速率变量



        const char* GetDataType() const;//参数类型
        const char* GetPosition() const;//端口位置
        const char* GetPutType() const;//输入输出类型
        const char* GetDescription();//描述
        const std::string& GetConfigKey() const;//配置键
        const char* GetHideCondition();//隐藏条件
        unsigned int GetRateValue() const;//速率变量名称
        const std::string& GetCodeGenPath() const;//代码生成名称
        std::string GetFullCodeGenName() const;//完整代码生成名称
        const std::vector<std::string>& GetRateVariableCodeGenNames();//速率变量的代码生成名称

        //验证端口配置
        bool Validate() const;
        //重置端口配置
        void Reset();
    };
    }
#endif // CDFPORTIMPLEMENTATION_H
