#include "configmanager.h"

#include <iostream>



void SystemVueModelBuilder::PortConfig::fromJson(const nlohmann::ordered_json &obj)
{
    //从json读取端口配置
    if(obj.contains("name")) name = obj.value("name","");
    position = obj.value("position","");
    if(obj.contains("putType")) putType = obj.value("putType","");
    if(obj.contains("dataType")) dataType = obj.value("dataType","");
    if(obj.contains("description")) description = obj.value("description","");
    if(obj.contains("hideCondition")) hideCondition = obj.value("hideCondition","");
    if(obj.contains("codeGenPath")) codeGenPath = obj.value("codeGenPath","");

    rateVariableNames.clear();
    if (obj.contains("rateVariableNames") && obj["rateVariableNames"].is_array()) {
        for (const auto& var : obj["rateVariableNames"]) {
            rateVariableNames.push_back(var.get<std::string>());
        }
    }
    isOptional = obj.value("isOptional", false);
    if(obj.contains("rateValue")) rateValue = obj.value("rateValue", 1);
}

nlohmann::ordered_json SystemVueModelBuilder::PortConfig::toOrderedJson() const
{
    //向json写入端口配置
    nlohmann::ordered_json obj;

    if(!name.empty()) obj["name"] = name;
    obj["pos"] = position;
    if(!dataType.empty()) obj["dataType"] = dataType;
    if(!putType.empty()) obj["putType"] = putType;
    if(!description.empty()) obj["description"] = description;
    if(!hideCondition.empty()) obj["hideCondition"] = hideCondition;
    if(!codeGenPath.empty()) obj["codeGenPath"] = codeGenPath;
    if(!rateVariableNames.empty()) obj["rateVariableNames"] = rateVariableNames;
    obj["isOptional"] = nlohmann::json(isOptional);
    obj["rateValue"] = rateValue;

    return obj;
}

void SystemVueModelBuilder::ParamConfig::fromJson(const nlohmann::ordered_json &obj)
{
    //从json读取参数配置
    if(obj.contains("name")) name = obj.value("name","");
    if(obj.contains("description")) description = obj.value("description","");
    if(obj.contains("dataType")) dataType = obj.value("dataType","");
    if(obj.contains("defaultValue")) defaultValue = obj.value("defaultValue","");
    if(obj.contains("val")) Value = obj.value("val","");
    // 从字符串转换为枚举
    std::string unitStr = obj.value("unit", "NONE");
    if (unitStr == "ANGLE") unit = Units::ANGLE;
    else if (unitStr == "LENGTH") unit = Units::LENGTH;
    else if (unitStr == "TIME") unit = Units::TIME;
    else if (unitStr == "FREQUENCY") unit = Units::FREQUENCY;
    else if (unitStr == "VOLTAGE") unit = Units::VOLTAGE;
    else if (unitStr == "POWER") unit = Units::POWER;
    else if (unitStr == "RESISTANCE") unit = Units::RESISTANCE;
    else if (unitStr == "TEMPERATURE") unit = Units::TEMPERATURE;
    else unit = Units::NONE;

    enumValues.clear();
    if (obj.contains("enumValues") && obj["enumValues"].is_array()) {
        for (const auto& enumValue : obj["enumValues"]) {
            enumValues.push_back(enumValue.get<std::string>());
        }
    }
    isAdvanced = obj.value("isAdvanced", false);
    hideCondition = obj.value("hideCondition", "");
}

nlohmann::ordered_json SystemVueModelBuilder::ParamConfig::toOrderedJson() const
{
    //向json写入参数配置
    nlohmann::ordered_json obj;

    if(!name.empty()) obj["name"] = name;
    if(!description.empty()) obj["description"] = description;
    if(!dataType.empty()) obj["dataType"] = dataType;
    if(!defaultValue.empty()) obj["defaultValue"] = defaultValue;
    if(!Value.empty()) obj["val"] = Value;
    std::string unitStr;
    switch (unit) {
    case Units::NONE: unitStr = "NONE"; break;
    case Units::ANGLE: unitStr = "ANGLE"; break;
    case Units::LENGTH: unitStr = "LENGTH"; break;
    case Units::TIME: unitStr = "TIME"; break;
    case Units::FREQUENCY: unitStr = "FREQUENCY"; break;
    case Units::VOLTAGE: unitStr = "VOLTAGE"; break;
    case Units::POWER: unitStr = "POWER"; break;
    case Units::RESISTANCE: unitStr = "RESISTANCE"; break;
    case Units::TEMPERATURE: unitStr = "TEMPERATURE"; break;
    default: unitStr = "NONE"; break;
    }
    obj["unit"] = unitStr;
    obj["enumValues"] = enumValues;
    obj["isAdvanced"] = nlohmann::json(isAdvanced);
    obj["hideCondition"] = hideCondition;

    return obj;
}


void SystemVueModelBuilder::ModelConfig::fromJson(const nlohmann::ordered_json &obj)
{
    //从json读取模型配置
    if(obj.contains("modelName")) modelname = obj.value("modelName", "");
    if(obj.contains("defaultModel")) defaultmodel = obj.value("defaultModel", "");
    if(obj.contains("modelDescription")) modeldescription = obj.value("modelDescription", "");
    if(obj.contains("modelCategory")) modelcategory = obj.value("modelCategory", "");
    if(obj.contains("modelSymbol")) modelsymbol = obj.value("modelSymbol", "");
    if(obj.contains("modelNamespace")) modelnamespace = obj.value("modelNamespace", "");
    if(obj.contains("modelCodeGenName")) modelcodegenname = obj.value("modelCodeGenName", "");
    if(obj.contains("customUI")) customUI = obj.value("customUI", "");

    headerfiles.clear();
    if (obj.contains("headerFiles") && obj["headerFiles"].is_array()) {
        for (const auto& headerfile : obj["headerFiles"]) {
            headerfiles.push_back(headerfile.get<std::string>());
        }
    }

    sourcefiles.clear();
    if (obj.contains("sourceFiles") && obj["sourceFiles"].is_array()) {
        for (const auto& sourcefile : obj["sourceFiles"]) {
            sourcefiles.push_back(sourcefile.get<std::string>());
        }
    }

    bPartGenerationEnabled = obj.value("PartGenerationEnabled", false);
    bModelGenerationEnabled = obj.value("ModelGenerationEnabled", false);
    bCppCodeGenerationEnabled = obj.value("CppCodeGenerationEnabled", false);
    bHidingIOEnabled = obj.value("HidingIOEnabled", false);
    bHidingIOFromParamEnabled = obj.value("HidingIOFromParamEnabled", false);

}

nlohmann::ordered_json SystemVueModelBuilder::ModelConfig::toOrderedJson() const
{
    //向json写入模型配置
    nlohmann::ordered_json obj;
    if(!modelname.empty()) obj["modelName"] = modelname;
    if(!defaultmodel.empty()) obj["defaultModel"] = defaultmodel;
    if(!modeldescription.empty()) obj["modelDescription"] = modeldescription;
    if(!modelcategory.empty()) obj["modelCategory"] = modelcategory;
    if(!modelsymbol.empty()) obj["modelSymbol"] = modelsymbol;
    if(!modelnamespace.empty()) obj["modelNamespace"] = modelnamespace;
    if(!modelcodegenname.empty()) obj["modelCodeGenName"] = modelcodegenname;
    if(!customUI.empty()) obj["customUI"] = customUI;
    if(!headerfiles.empty()) obj["headerFiles"] = headerfiles;
    if(!sourcefiles.empty()) obj["sourceFiles"] = sourcefiles;
    obj["PartGenerationEnabled"] = nlohmann::json(bPartGenerationEnabled);
    obj["ModelGenerationEnabled"] = nlohmann::json(bModelGenerationEnabled);
    obj["CppCodeGenerationEnabled"] = nlohmann::json(bCppCodeGenerationEnabled);
    obj["HidingIOEnabled"] = nlohmann::json(bHidingIOEnabled);
    obj["HidingIOFromParamEnabled"] = nlohmann::json(bHidingIOFromParamEnabled);

    return obj;
}
SystemVueModelBuilder::ConfigManager::ConfigManager() :m_autoSave(false)
{
    //初始化
    Initialize();
}

SystemVueModelBuilder::ConfigManager::ConfigManager(const std::string &configFile)
    :m_configFile(configFile),m_autoSave(false)
{
    //初始化
    Initialize();
    LoadFromFile(configFile);
}

SystemVueModelBuilder::ConfigManager::~ConfigManager()
{
    if(m_autoSave) {
        SaveToFile();
    }
}
//
void SystemVueModelBuilder::ConfigManager::SetProjectData(const std::string &cmpType, const std::string &objectType,
                                                          const std::string &icon, const std::string &iconObject,
                                                          const std::string &instanceName, const std::string &ADSLib)
{
    //设置额外字段
    m_cmpType = cmpType;
    m_objectType = objectType;
    m_icon = icon;
    m_iconObject = iconObject;
    m_instanceName = instanceName;
    m_ADSLib = ADSLib;
}

std::string SystemVueModelBuilder::ConfigManager::GetCmpType() const
{
    //获取cmptype
    return m_cmpType;
}

std::string SystemVueModelBuilder::ConfigManager::GetObjectType() const
{
    //获取objecttype
    return m_objectType;
}

std::string SystemVueModelBuilder::ConfigManager::GetIcon() const
{
    //获取图标
    return m_icon;
}

std::string SystemVueModelBuilder::ConfigManager::GetIconObject() const
{
    //获取图标文件
    return m_iconObject;
}

std::string SystemVueModelBuilder::ConfigManager::GetInstanceName() const
{
    //获取InstanceName
    return m_instanceName;
}

std::string SystemVueModelBuilder::ConfigManager::GetADSLib() const
{
    //获取ADSLib
    return m_ADSLib;
}
// DFPort 接口实现
void SystemVueModelBuilder::ConfigManager::SavePortConfig(const std::string &portKey, const PortConfig &config)
{
    //保存端口配置
    if(portKey.empty()) return;
    m_portConfigs[portKey] = config;
    if(m_autoSave) {
        SaveToFile();
    }
}
SystemVueModelBuilder::PortConfig SystemVueModelBuilder::ConfigManager::LoadPortConfig(const std::string &portKey) const
{
    //加载端口配置
    auto it = m_portConfigs.find(portKey);
    if(it != m_portConfigs.end()) {
        return it->second;
    }
    //返回默认配置
    return PortConfig();
}
bool SystemVueModelBuilder::ConfigManager::HasPortConfig(const std::string &portKey) const
{
    //检查端口配置
    return m_portConfigs.find(portKey) != m_portConfigs.end();
}
void SystemVueModelBuilder::ConfigManager::RemovePortConfig(const std::string &portKey)
{
    //移除端口配置
    m_portConfigs.erase(portKey);
    if(m_autoSave) {
        SaveToFile();
    }
}
std::unordered_map<std::string, SystemVueModelBuilder::PortConfig> SystemVueModelBuilder::ConfigManager::GetAllPortConfigs() const
{
    return m_portConfigs;
}
// DFParam 接口实现
void SystemVueModelBuilder::ConfigManager::SaveParamConfig(const std::string &paramKey, const ParamConfig &config)
{
    //加载参数配置
    if(paramKey.empty()) return;
    m_paramConfigs[paramKey] = config;
    if(m_autoSave) {
        SaveToFile();
    }
}
SystemVueModelBuilder::ParamConfig SystemVueModelBuilder::ConfigManager::LoadParamConfig(const std::string &paramKey)
{
    //加载参数配置
    auto it = m_paramConfigs.find(paramKey);
    if(it != m_paramConfigs.end()) {
        return it->second;
    }
    //返回默认配置
    return ParamConfig();
}
bool SystemVueModelBuilder::ConfigManager::HasParamConfig(const std::string &paramKey)
{
    //检查参数配置
    return m_paramConfigs.find(paramKey) != m_paramConfigs.end();
}
void SystemVueModelBuilder::ConfigManager::RemoveParamConfig(const std::string &paramKey)
{
    //移除参数配置
    m_paramConfigs.erase(paramKey);
    if(m_autoSave) {
        SaveToFile();
    }
}
std::unordered_map<std::string, SystemVueModelBuilder::ParamConfig> SystemVueModelBuilder::ConfigManager::GetAllParamConfigs() const
{
    return m_paramConfigs;
}
//DFModel 接口实现
void SystemVueModelBuilder::ConfigManager::SaveModelConfig(const std::string &modelKey, const ModelConfig &config)
{
    //保存模型配置
    if(modelKey.empty()) return;
    m_modelConfigs[modelKey] = config;
    if(m_autoSave) {
        SaveToFile();
    }
}
SystemVueModelBuilder::ModelConfig SystemVueModelBuilder::ConfigManager::LoadModelConfig(const std::string &modelKey)
{
    //加载模型配置
    auto it = m_modelConfigs.find(modelKey);
    if(it != m_modelConfigs.end()) {
        return it->second;
    }
    return ModelConfig();
}
bool SystemVueModelBuilder::ConfigManager::HasModelConfig(const std::string &modelKey)
{
    //检查模型配置
    return m_modelConfigs.find(modelKey) != m_modelConfigs.end();
}
void SystemVueModelBuilder::ConfigManager::RemoveModelConfig(const std::string &modelKey)
{
    //移除模型配置
    m_modelConfigs.erase(modelKey);
    if(m_autoSave)
        SaveToFile();
}
std::unordered_map<std::string, SystemVueModelBuilder::ModelConfig> SystemVueModelBuilder::ConfigManager::GetAllModelConfigs() const
{
    return m_modelConfigs;
}
// 文件操作实现
bool SystemVueModelBuilder::ConfigManager::SaveToFile(const std::string &filename)
{
    //保存到文件
    std::string fileToUse = filename.empty() ? m_configFile : filename;

    if(fileToUse.empty()) {
        return false;
    }

    try {
        nlohmann::ordered_json configJson = ExportConfig();
        // 保存到文件
        std::ofstream file(fileToUse);
        file << configJson.dump(4); // 缩进4个空格
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存文件配置失败" << e.what() << std::endl;
        return false;
    }
}

bool SystemVueModelBuilder::ConfigManager::LoadFromFile(const std::string &filename)
{
    //加载文件
    std::string fileToUse = filename.empty() ? m_configFile : filename;

    if(fileToUse.empty()) {
        return false;
    }
    try {
        std::ifstream file(fileToUse);
        if(!file.is_open()) {
            return false;
        }

        nlohmann::json config;
        file >> config;
        file.close();

        return ImportConfig(config  );
    } catch (const std::exception& e) {
        std::cerr << "加载文件配置失败" << e.what() << std::endl;
        return false;
    }
}
void SystemVueModelBuilder::ConfigManager::SetAutoSave(bool autoSave)
{
    //设置自动保存
    m_autoSave = autoSave;
}
std::string SystemVueModelBuilder::ConfigManager::GetConfigFile() const
{
    return m_configFile;
}
void SystemVueModelBuilder::ConfigManager::ClearAll()
{
    //清空配置
    m_portConfigs.clear();
    m_paramConfigs.clear();
}

bool SystemVueModelBuilder::ConfigManager::ValidateConfig() const
{
    //判断所有配置有效
    for(const auto& [_,config] : m_portConfigs) {
        if(!ValidatePortConfig(config)) {
            return false;
        }
    }
    for(const auto& [_,config] : m_paramConfigs) {
        if(!ValidateParamConfig(config)) {
            return false;
        }
    }
    return true;
}

bool SystemVueModelBuilder::ConfigManager::ImportConfig(const nlohmann::ordered_json &config)
{
    //输入配置
    try {
        //项目
        m_cmpType = config.value("cmpType","");
        m_objectType = config.value("objectType","");
        m_icon = config.value("icon","");
        m_iconObject = config.value("iconObject","");
        m_instanceName = config.value("instanceName","");
        m_ADSLib = config.value("ADSLib","");

        //模型
        m_modelConfigs.clear();
        if(config.contains("model") && config["model"].is_object())
        {
            for(auto& [key,value] : config["model"].items()) {
                ModelConfig modelConfig;
                modelConfig.fromJson(value);
                m_modelConfigs[key] = modelConfig;
            }
        }
        //端口
        m_portConfigs.clear();
        if(config.contains("ports") && config["ports"].is_object())
        {
            for(auto& [key,value] : config["ports"].items()) {
                PortConfig portConfig;
                portConfig.fromJson(value);
                m_portConfigs[key] = portConfig;
            }
        }
        //参数
        m_paramConfigs.clear();
        //if(config.contains("attribute") && config["attribute"].is_object())
        if(config.contains("attribute") && config["attribute"].is_array())
        {
            for(const auto& paramObj : config["attribute"]) {
                ParamConfig paramConfig;
                paramConfig.fromJson(paramObj);

                std::string key;
                if(paramObj.contains("key") && paramObj["key"].is_string()) {
                    key = paramObj["key"].get<std::string>();
                }else {
                    key = paramConfig.name;
                }
                if(!key.empty()) {
                    m_paramConfigs[key] = paramConfig;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "导入配置失败: " << e.what() << std::endl;
        return false;
    }
}

nlohmann::ordered_json SystemVueModelBuilder::ConfigManager::ExportConfig() const
{
    //输出配置
    nlohmann::ordered_json configJson;
    // 添加项目数据
    configJson["cmpType"] = m_cmpType;
    configJson["objectType"] = m_objectType;
    configJson["icon"] = m_icon;
    configJson["iconObject"] = m_iconObject;
    configJson["instanceName"] = m_instanceName;
    configJson["ADSLib"] = m_ADSLib;

    nlohmann::ordered_json modelJson;
    for(const auto& [key,config] : m_modelConfigs) {
        modelJson[key] = config.toOrderedJson();
    }
    configJson["model"] = modelJson;

    nlohmann::ordered_json portsJson;
    for(const auto& [key,config] : m_portConfigs) {
        portsJson[key] = config.toOrderedJson();
    }
    configJson["port"] = portsJson;

    //参数部分为数组形式
    nlohmann::ordered_json paramsArray = nlohmann::ordered_json::array();

    for(const auto& [_,config] : m_paramConfigs) {
        nlohmann::ordered_json paramObj = config.toOrderedJson();

        //paramObj["key"] = key;
        paramsArray.push_back(paramObj);  // 添加到数组
    }

    configJson["attribute"] = paramsArray;  // 数组形式

    return configJson;
}

void SystemVueModelBuilder::ConfigManager::Initialize()
{
    //初始化
    m_portConfigs.clear();
    m_paramConfigs.clear();
    m_autoSave = false;
}

bool SystemVueModelBuilder::ConfigManager::ValidatePortConfig(const PortConfig &config) const
{
    //判断端口配置有效
    if(config.name.empty()) {
        return false;
    }
    return true;
}

bool SystemVueModelBuilder::ConfigManager::ValidateParamConfig(const ParamConfig &config) const
{

    //判断参数配置有效
    if(config.name.empty()) {
        return false;
    }
    return true;
}










