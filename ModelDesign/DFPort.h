#ifdef _MSC_VER
    #pragma warning(disable:4819)
#endif
#ifndef DFPORT_H
#define DFPORT_H
#pragma once

#include "configmanager.h"

namespace SystemVueModelBuilder {
    //用于DFPort参数的实现
    class CDFPortImplementation;

    class DFPort
    {
    private:
        CDFPortImplementation *m_cPortImplementation;
        std::shared_ptr<ConfigManager> m_configManager;
        void SaveConfigToManager();
    public:
        DFPort();
        DFPort(CDFPortImplementation *cPortImplementation);
        DFPort(const DFPort& other);
        ~DFPort();
        // 交换函数
        friend void swap(DFPort& first, DFPort& second) noexcept {
            using std::swap;
            swap(first.m_cPortImplementation, second.m_cPortImplementation);
            swap(first.m_configManager, second.m_configManager);
        }

        // 复制赋值运算符（copy-and-swap）
        DFPort& operator=(DFPort other) {  // 按值传参，调用复制构造函数
            swap(*this, other);
            return *this;
        }

        //SystemVue 端口定义接口
        void SetName(const char * pcName);
        void SetPutType(const char* pcPutType);
        void SetPosition(const char* pcPosition);
        void SetDataType(const char* pcDataType);
        void SetOptional(bool bIsOptional = true); //设置是否可选
        void SetRateValue(unsigned int rateVariable);//设置速率大小
        void SetDescription(const char *pcDescription);
        void SetHideCondition(const char * pcHideCondition);
        void AddRateVariable( unsigned &iRate);
        void AddRateVariableCodeGenName( const char* pccVariableName);
        void PrependCodeGenName(const char * pcCodeGenPath);

        void SetConfigManager(std::shared_ptr<ConfigManager> manager);
        CDFPortImplementation* GetImplementation() const;
    };
    }
#endif // DFPORT_H
