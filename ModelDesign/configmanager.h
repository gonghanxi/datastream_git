#ifdef _MSC_VER
    #pragma warning(disable:4819)
#endif
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>

#include "DFEnumerations.h"
#include "json.hpp"
#include <fstream>


namespace SystemVueModelBuilder {
    //端口配置结构体
    struct PortConfig
    {
        std::string name;
        std::string description;
        std::string dataType;
        std::string hideCondition;
        std::string codeGenPath;
        std::vector<std::string> rateVariableNames;
        bool isOptional;
        //unsigned rateValue;
        unsigned int rateValue;
        std::string position;
        std::string putType;

        //从json读取数据
        void fromJson(const nlohmann::ordered_json& obj);

        //写入json
        nlohmann::ordered_json toOrderedJson() const;
    };
    //参数配置结构体
    struct ParamConfig
    {
        std::string name;
        std::string description;
        std::string dataType;
        std::string defaultValue;
        std::string Value;
        Units::UnitType unit;
        std::vector<std::string> enumValues;
        std::string hideCondition;
        bool isAdvanced;

        //从json读取数据
        void fromJson(const nlohmann::ordered_json& obj);

        //写入json
        nlohmann::ordered_json toOrderedJson() const;
    };

    //模型配置结构体
    struct ModelConfig
    {
        std::string modelname;
        std::string defaultmodel;
        std::string modeldescription;
        std::string modelcategory;
        std::string modelsymbol;
        std::string modelnamespace;
        std::string modelcodegenname;
        std::string customUI; //用户UI

        std::vector<std::string> headerfiles;
        std::vector<std::string> sourcefiles;

        bool bPartGenerationEnabled; //是否允许部分生成
        bool bModelGenerationEnabled; //是否允许模型生成
        bool bCppCodeGenerationEnabled; //是否允许C++代码生成
        bool bHidingIOEnabled; //是否允许隐藏IO
        bool bHidingIOFromParamEnabled; //是否允许隐藏参数IO

        void fromJson(const nlohmann::ordered_json& obj);

        nlohmann::ordered_json toOrderedJson() const;
    };

    class ConfigManager
    {
    private:
        //配置映射
        std::unordered_map<std::string,PortConfig>m_portConfigs;
        std::unordered_map<std::string,ParamConfig>m_paramConfigs;
        std::unordered_map<std::string,ModelConfig>m_modelConfigs;
        //配置文件名称
        std::string m_configFile;
        bool m_autoSave;

        //json额外字段
        std::string m_cmpType;
        std::string m_objectType;
        std::string m_icon;
        std::string m_iconObject;
        std::string m_instanceName;
        std::string m_ADSLib;

    public:
        ConfigManager();
        explicit ConfigManager(const std::string& configFile);
        ~ConfigManager();
        // ========== 项目数据相关接口 ==========
        void SetProjectData(const std::string& cmpType, const std::string& objectType,
                            const std::string& icon, const std::string& iconObject,
                            const std::string& instanceName, const std::string& ADSLib);

        std::string GetCmpType() const;
        std::string GetObjectType() const;
        std::string GetIcon() const;
        std::string GetIconObject() const;
        std::string GetInstanceName() const;
        std::string GetADSLib() const;
        // ========== DFPort 相关接口 ==========
        //保存配置
        void SavePortConfig(const std::string& portKey, const PortConfig& config);
        //加载配置
        PortConfig LoadPortConfig(const std::string& portKey) const;
        //检查端口配置是否存在
        bool HasPortConfig(const std::string& portKey) const;
        //删除配置
        void RemovePortConfig(const std::string& portKey);
        //获取所有端口配置
        std::unordered_map<std::string,PortConfig> GetAllPortConfigs() const;
        // ========== DFParam 相关接口 ==========
        //保存
        void SaveParamConfig(const std::string& paramKey, const ParamConfig& config);
        //加载
        ParamConfig LoadParamConfig(const std::string& paramKey);
        //检查参数配置是否存在
        bool HasParamConfig(const std::string& paramKey);
        //删除配置
        void RemoveParamConfig(const std::string& paramKey);
        //获取所有参数配置
        std::unordered_map<std::string,ParamConfig> GetAllParamConfigs() const;
        // ========== DFModel 相关接口 ==========
        //保存
        void SaveModelConfig(const std::string& modelKey,const ModelConfig& config);
        //加载
        ModelConfig LoadModelConfig(const std::string& modelKey);
        //检查参数配置是否存在
        bool HasModelConfig(const std::string& modelKey);
        //删除
        void RemoveModelConfig(const std::string& modelKey);
        //获取所有参数配置
        std::unordered_map<std::string,ModelConfig> GetAllModelConfigs() const;
        // ========== 文件操作接口 ==========
        //保存所有配置到文件
        bool SaveToFile(const std::string& filename = "");
        //从文件加载所有配置
        bool LoadFromFile(const std::string& filename = "");
        //设置自动保存
        void SetAutoSave(bool autoSave);
        //获取当前配置文件路径
        std::string GetConfigFile() const;
        //清除所有配置
        void ClearAll();


        //验证配置完整性
        bool ValidateConfig() const;
        //导入配置
        bool ImportConfig(const nlohmann::ordered_json& config);
        //导出配置
        nlohmann::ordered_json ExportConfig() const;
    private:
        //内部初始化
        void Initialize();
        //生成默认配置
        void CreateDefaultConfig();
        //验证端口配置
        bool ValidatePortConfig(const PortConfig& config) const;
        //验证参数配置
        bool ValidateParamConfig(const ParamConfig& config) const;
    };
    }
#endif // CONFIGMANAGER_H



















