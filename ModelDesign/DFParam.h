#ifndef DFPARAM_H
#define DFPARAM_H
#pragma once

#include "DFEnumerations.h"

#include "configmanager.h"
namespace SystemVueModelBuilder {
    class CDFParamImplementation;
    class DFParam
    {
    private:
        //实现类指针
        CDFParamImplementation *m_cParamImplementation;
        //配置保存指针
        std::shared_ptr<ConfigManager> m_configManager;
        void SaveConfigToManager();
    public:
        DFParam();
        DFParam(CDFParamImplementation *cParamImplementation);
        DFParam(const DFParam& other);
        ~DFParam();
        // 交换函数
        friend void swap(DFParam& first, DFParam& second) noexcept {
            using std::swap;
            swap(first.m_cParamImplementation, second.m_cParamImplementation);
            swap(first.m_configManager, second.m_configManager);
        }

        // 复制赋值运算符（copy-and-swap）
        DFParam& operator=(DFParam other) {
            swap(*this, other);
            return *this;
        }

        //SystemVue 参数定义接口
        void SetName(const char* pcName);
        void SetDescription(const char *pcDescription);
        void SetDefaultValue(const char *pcValue);
        void SetValue(const char *pcValue);
        void SetUnit(Units::UnitType eUnitType);
        void SetParamAsFile();
        void AddEnumeration(const char *pcEnumName, int iEnumValue);// 添加枚举参数
        void SetEnumeration(const char * EnumerationName); //设置枚举参数
        void PrependCodeGenName(const char * pcCodeGenPath);
        void SetHideCondition(const char * pcHideCondition);
        void SetSchematicDisplay( bool bDisplay); //是否视图显示
        void SetNonSetable(bool bNonSetable); //是否禁止设置
        void SetDynamicUpdate( bool bDynamicUpdateSupported); //是否支持动态更新
        void SetUseDefault(bool bFlag); //是否使用默认值

        void SetConfigManager(std::shared_ptr<ConfigManager> manager);
        CDFParamImplementation* GetImplementation() const;
    };
    }
#endif // DFPARAM_H
