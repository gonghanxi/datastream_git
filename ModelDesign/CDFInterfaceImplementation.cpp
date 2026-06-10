#include "CDFInterfaceImplementation.h"
#include "configmanager.h"
#include "StringUtils.h"



using namespace SystemVueModelBuilder;

    CDFInterfaceImplementation::CDFInterfaceImplementation()
        : m_bPartGenerationEnabled(true)
        , m_bModelGenerationEnabled(true)
        , m_bCppCodeGenerationEnabled(true)
        , m_bHidingIOEnabled(false)
        , m_bHidingIOFromParamEnabled(false)
    {
        //初始化
        m_configManager = std::make_shared<ConfigManager>();

        m_configKey = "default_model_config";

        // if(m_configManager) {
        //     m_configManager->SaveToFile("E:/project/archermind/interface_started.json");
        //     std::ofstream marker("E:/project/archermind/interface_implementation_created.txt");
        //     if(marker.is_open()) {
        //         marker << "CDFInterfaceImplementation constructor completed";
        //         marker.close();
        //     }
        // }
         m_configManager->SetAutoSave(true);
    }

    CDFInterfaceImplementation::CDFInterfaceImplementation(const std::string &configKey)
        : m_bPartGenerationEnabled(true)
        , m_bModelGenerationEnabled(true)
        , m_bCppCodeGenerationEnabled(true)
        , m_bHidingIOEnabled(false)
        , m_bHidingIOFromParamEnabled(false)
        , m_configKey(configKey)
    {
        //初始化
        m_configManager = std::make_shared<ConfigManager>();
         m_configManager->SetAutoSave(true);
    }

    bool CDFInterfaceImplementation::HasConfigManager() const
    {
        return m_configManager != nullptr;
    }

    bool CDFInterfaceImplementation::SaveCompleteConfig(const std::string &filename)
    {
        if(!m_configManager) {
            m_lastError = "Config manager is not available";
            return false;
        }

        try {
            //获取模型配置
            ModelConfig modelConfig = ToModelConfig();
            m_configManager->SaveModelConfig(m_configKey, modelConfig);


            //设置输出文件
            std::string fileToUse = filename.empty() ?
                                        "E:/project/archermind/complete_config.json" : filename;

            return m_configManager->SaveToFile(fileToUse);
        } catch (const std::exception& e) {
            m_lastError = std::string("Failed to save complete config: ") + e.what();
            return false;
        }
    }


    void CDFInterfaceImplementation::SetConfigManager(std::shared_ptr<ConfigManager> manager)
    {
        //设置指针
        m_configManager = manager;
    }

    std::shared_ptr<ConfigManager> CDFInterfaceImplementation::GetConfigManager() const
    {
        return m_configManager;
    }

    void CDFInterfaceImplementation::SavePortConfig(const std::string &portKey, const PortConfig &config)
    {
        //保存端口配置
        if(m_configManager) {
            m_configManager->SavePortConfig(portKey, config);
        }
    }

    void CDFInterfaceImplementation::SaveParamConfig(const std::string &paramKey, const ParamConfig &config)
    {
        //保存参数配置
        if(m_configManager) {
            m_configManager->SaveParamConfig(paramKey, config);
        }
    }

    ModelConfig CDFInterfaceImplementation::LoadModelConfig()
    {
        //加载模型配置
        if(m_configManager) {
            return m_configManager->LoadModelConfig(m_configKey);
        }
        return ModelConfig();
    }

    void CDFInterfaceImplementation::SaveModelConfig(const ModelConfig &config)
    {
        //保存模型配置
        if(m_configManager) {
            return m_configManager->SaveModelConfig(m_configKey, config);
        }
    }



    void CDFInterfaceImplementation::SetConfigKey(const std::string &configKey)
    {
        //设置配置键
        m_configKey = configKey;
    }

    const std::string &CDFInterfaceImplementation::GetConfigKey() const
    {
        return m_configKey;
    }

    bool CDFInterfaceImplementation::SaveToConfig(ConfigManager &externalManager)
    {
        //保存配置接口
        if(m_configKey.empty()) return false;
        ModelConfig config = ToModelConfig();
        externalManager.SaveModelConfig(m_configKey,config);
        return true;
    }

    bool CDFInterfaceImplementation::LoadFromConfig()
    {
        //加载配置接口
        if(m_configKey.empty()) return false;
        if(m_configManager->HasModelConfig(m_configKey)) {
            ModelConfig config = m_configManager->LoadModelConfig(m_configKey);
            FromModelConfig(config);
            return true;
        }
        return false;
    }

    bool CDFInterfaceImplementation::RemoveFromConfig()
    {
        //移除配置
        if(m_configKey.empty()) return false;
        m_configManager->RemoveModelConfig(m_configKey);
        return true;
    }

    bool CDFInterfaceImplementation::HasConfig() const
    {
        return !m_configKey.empty() && m_configManager->HasModelConfig(m_configKey);
    }

    void CDFInterfaceImplementation::FromModelConfig(const ModelConfig &config)
    {
        //读取模型配置到成员变量
        m_modelName = config.modelname;
        m_defaultModel = config.defaultmodel;
        m_modelDescription = config.modeldescription;
        m_modelCategory = config.modelcategory;
        m_modelSymbol = config.modelsymbol;
        m_modelNamespace = config.modelnamespace;
        m_modelCodeGenName = config.modelcodegenname;
        m_customUI = config.customUI;

        m_headerFiles.clear();
        for(size_t i = 0; i < config.headerfiles.size(); i++) {
            m_headerFiles.emplace_back(config.headerfiles[i], static_cast<int>(i));
        }
        m_sourceFiles.clear();
        for(size_t i = 0; i < config.sourcefiles.size(); i++) {
            m_sourceFiles.emplace_back(config.sourcefiles[i], static_cast<int>(i));
        }
        m_bPartGenerationEnabled = config.bPartGenerationEnabled;
        m_bModelGenerationEnabled = config.bModelGenerationEnabled;
        m_bCppCodeGenerationEnabled = config.bCppCodeGenerationEnabled;
        m_bHidingIOEnabled = config.bHidingIOEnabled;
        m_bHidingIOFromParamEnabled = config.bHidingIOFromParamEnabled;
    }

    ModelConfig CDFInterfaceImplementation::ToModelConfig() const
    {
        //写入模型配置
        ModelConfig config;
        config.modelname = m_modelName;
        config.defaultmodel = m_defaultModel;
        config.modeldescription = m_modelDescription;
        config.modelcategory = m_modelCategory;
        config.modelsymbol = m_modelSymbol;
        config.modelnamespace = m_modelNamespace;
        config.modelcodegenname = m_modelCodeGenName;
        config.customUI = m_customUI;

        config.headerfiles.reserve(m_headerFiles.size());
        for(const auto& headerfile : m_headerFiles) {
            config.headerfiles.push_back(headerfile);
        }
        config.sourcefiles.reserve(m_sourceFiles.size());
        for(const auto& sourcefile : m_sourceFiles) {
            config.sourcefiles.push_back(sourcefile);
        }
        config.bPartGenerationEnabled = m_bPartGenerationEnabled;
        config.bModelGenerationEnabled = m_bModelGenerationEnabled;
        config.bCppCodeGenerationEnabled = m_bCppCodeGenerationEnabled;
        config.bHidingIOEnabled = m_bHidingIOEnabled;
        config.bHidingIOFromParamEnabled = m_bHidingIOFromParamEnabled;

        return config;
    }

    void CDFInterfaceImplementation::AddModel(const char *pcModelName)
    {
        //添加模型
        if(pcModelName)
            m_modelName = pcModelName;
    }

    void CDFInterfaceImplementation::SetDefaultModel(const char *pcModelName)
    {
        //设置默认模型
        if(pcModelName)
            m_defaultModel = pcModelName;
    }

    void CDFInterfaceImplementation::SetModelDescription(const char *pcDescription)
    {
        //设置模型描述
        if(pcDescription)
            m_modelDescription = pcDescription;
        if(m_configManager) {
            ModelConfig modelConfig = ToModelConfig();
            m_configManager->SaveModelConfig(m_configKey, modelConfig);
        }
    }

    void CDFInterfaceImplementation::SetModelCategory(const char *pcCategory)
    {
        //设置模型目录
        if(pcCategory)
            m_modelCategory = pcCategory;
        if(m_configManager) {
            ModelConfig modelConfig = ToModelConfig();
            m_configManager->SaveModelConfig(m_configKey, modelConfig);

        }
    }

    void CDFInterfaceImplementation::SetModelSymbol(const char *pcSymbolName)
    {
        //设置模型symbol
        if(pcSymbolName)
            m_modelSymbol = pcSymbolName;
        if(m_configManager) {
            ModelConfig modelConfig = ToModelConfig();
            m_configManager->SaveModelConfig(m_configKey, modelConfig);

            //m_configManager->SaveToFile("E:/project/archermind/model_symbol_set.json");
        }
    }

    void CDFInterfaceImplementation::SetModelName(const char *pcName)
    {
        if(pcName)
            m_modelName = pcName;
    }

    void CDFInterfaceImplementation::SetModelNamespace(const char *pcNamespace)
    {
        //设置模型命名空间
        if(pcNamespace)
            m_modelNamespace = pcNamespace;
    }

    void CDFInterfaceImplementation::SetModelCodeGenName(const char *pcName)
    {
        //设置模型代码生成名称
        if(pcName)
            m_modelCodeGenName = pcName;
    }

    void CDFInterfaceImplementation::SetCustomUI(const char *pcCustomUIName)
    {
        //设置模型用户ui
        if(pcCustomUIName)
            m_customUI = pcCustomUIName;
    }

    void CDFInterfaceImplementation::DisablePartGeneration()
    {
        //禁用部分生成
        m_bPartGenerationEnabled = false;
    }

    void CDFInterfaceImplementation::DisablePartAndModelGeneration()
    {
        //禁用部分和模型生成
        m_bPartGenerationEnabled = false;
        m_bModelGenerationEnabled = false;
    }

    void CDFInterfaceImplementation::EnablePartAndModelGeneration()
    {
        //启用部分和模型生成
        m_bPartGenerationEnabled = true;
        m_bModelGenerationEnabled = true;
    }

    void CDFInterfaceImplementation::DisableCppCodeGeneration()
    {
        //禁用C++代码生成
        m_bCppCodeGenerationEnabled = false;
    }

    void CDFInterfaceImplementation::EnableHidingIO()
    {
        //启用隐藏IO
        m_bHidingIOEnabled = true;
    }

    void CDFInterfaceImplementation::EnableHidingIOFromParam()
    {
        //启用隐藏参数IO
        m_bHidingIOFromParamEnabled = true;
    }

    void CDFInterfaceImplementation::AddModelHeaderFile(const char *pcHeaderFile)
    {
        //添加模型头文件
        if(pcHeaderFile)
            m_headerFiles.push_back(pcHeaderFile);
    }

    void CDFInterfaceImplementation::AddModelSourceFile(const char *pcSourceFile)
    {
        //添加模型源文件
        if(pcSourceFile)
            m_sourceFiles.push_back(pcSourceFile);
    }

    void CDFInterfaceImplementation::RegisterSimulationControl(const SinkControl &sinkControl, const char *pcCodeGenName)
    {
        //注册模拟终端控制
        if(pcCodeGenName) {
            m_sinkControls.try_emplace(pcCodeGenName, sinkControl);
        }
    }

    void CDFInterfaceImplementation::RegisterSimulationControl(const DynamicControl &dynamicControl, const char *pcCodeGenName)
    {
        //注册模型动态控制
        if(pcCodeGenName) {
            m_dynamicControls.try_emplace(pcCodeGenName, dynamicControl);
        }
    }

    DFParam CDFInterfaceImplementation::AddParam(double &dData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(dData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(float &dData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(dData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParamArray(double *&pdData, int &iSize, const char *pcCodeGenName, const char *pcSizeName)
    {
        //添加数组参数
        if(!pcCodeGenName || !pcSizeName) {
            m_lastError = "Code generation name and size parameter name are required";
            return DFParam(nullptr);
        }
        try {
            CDFParamImplementation* paramImpl = new CDFParamImplementation();

            //设置
            paramImpl->SetName(pcCodeGenName);
            paramImpl->SetParamType("double[]");
            paramImpl->SetSizeParameterName(pcSizeName);

            if(!m_modelName.empty()) {
                paramImpl->PrependCodeGenName(m_modelName.c_str());
            }

            //判断是否有效
            if(!paramImpl->Validate()) {
                delete paramImpl;
                m_lastError = "Array parameter validation failed";
                return DFParam(nullptr);
            }

            //添加新数组并初始化
            if(!pdData && iSize > 0) {
                pdData = new double[iSize];
                std::fill(pdData,pdData + iSize,0.0);
            }
            DFParam dfParam(paramImpl);
            m_params.push_back(dfParam);
            return dfParam;
        } catch (const std::exception& e) {
            m_lastError = std::string("Failed to create array parameter: ") + e.what();
            return DFParam(nullptr);
        }
    }

    DFParam CDFInterfaceImplementation::AddParam(int &iData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(iData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParamArray(int *&piData, int &iSize, const char *pcCodeGenName, const char *pcSizeName)
    {
        //添加数组参数
        if(!pcCodeGenName || !pcSizeName) {
            m_lastError = "Code generation name and size parameter name are required";
            return DFParam(nullptr);
        }
        try {
            CDFParamImplementation* paramImpl = new CDFParamImplementation();

            //设置
            paramImpl->SetName(pcCodeGenName);
            paramImpl->SetParamType("int[]");
            paramImpl->SetSizeParameterName(pcSizeName);

            if(!m_modelName.empty()) {
                paramImpl->PrependCodeGenName(m_modelName.c_str());
            }

            //判断是否有效
            if(!paramImpl->Validate()) {
                delete paramImpl;
                m_lastError = "Array parameter validation failed";
                return DFParam(nullptr);
            }

            //添加新数组并初始化
            if(!piData && iSize > 0) {
                piData = new int[iSize];
                std::fill(piData,piData + iSize,0.0);
            }
            DFParam dfParam(paramImpl);
            m_params.push_back(dfParam);
            return dfParam;
        } catch (const std::exception& e) {
            m_lastError = std::string("Failed to create array parameter: ") + e.what();
            return DFParam(nullptr);
        }
    }



    DFParam CDFInterfaceImplementation::AddParam(std::complex<float> &cComplexData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cComplexData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(std::complex<double> &cComplexData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cComplexData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParamArray(std::complex<double> *&pcComplexData, int &iSize, const char *pcCodeGenName, const char *pcSizeName)
    {
        //添加数组参数
        if(!pcCodeGenName || !pcSizeName) {
            m_lastError = "Code generation name and size parameter name are required";
            return DFParam(nullptr);
        }
        try {
            CDFParamImplementation* paramImpl = new CDFParamImplementation();

            //设置
            paramImpl->SetName(pcCodeGenName);
            paramImpl->SetParamType("double[]");
            paramImpl->SetSizeParameterName(pcSizeName);

            if(!m_modelName.empty()) {
                paramImpl->PrependCodeGenName(m_modelName.c_str());
            }

            //判断是否有效
            if(!paramImpl->Validate()) {
                delete paramImpl;
                m_lastError = "Array parameter validation failed";
                return DFParam(nullptr);
            }

            //添加新数组并初始化
            if(!pcComplexData && iSize > 0) {
                pcComplexData = new std::complex<double>[iSize];
                std::fill(pcComplexData,pcComplexData + iSize,0.0);
            }
            DFParam dfParam(paramImpl);
            m_params.push_back(dfParam);
            return dfParam;
        } catch (const std::exception& e) {
            m_lastError = std::string("Failed to create array parameter: ") + e.what();
            return DFParam(nullptr);
        }
    }

    DFParam CDFInterfaceImplementation::AddParam(char *&sData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(sData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(QueryEnum &qData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(qData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(BooleanEnum &bData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(bData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(SwitchEnum &sData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(sData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParamFile(const char *pcName, const char *pcDescription, char *&sData, const char *pcValue)
    {
        //添加文件参数
        if( !pcName || !pcDescription) {
            m_lastError = "Invalid parameter data or code generation name";
            return DFParam(nullptr);
        }
        try {
            CDFParamImplementation* paramImpl = new CDFParamImplementation();

            //设置
            paramImpl->SetName(pcName);
            paramImpl->SetDescription(pcDescription);

            paramImpl->SetParamType("string");
            paramImpl->SetParamAsFile();

            if(pcValue) {
                paramImpl->SetDefaultValue(pcValue);

                delete[] sData;
                size_t len = strlen(pcValue) + 1;
                sData = new char[len];
                StringUtils::strcpy_safe(sData, len, pcValue);
            }
            else {
                delete[] sData;
                sData = new char[1];
                sData[0] = '\0';
            }
            paramImpl->PrependCodeGenName(pcName);

            DFParam param(paramImpl);
            m_params.push_back(param);
            return param;

        } catch (const std::exception& e) {
            m_lastError = std::string("Failed to create parameter: ") + e.what();
            return DFParam(nullptr);
        }
    }

    DFParam CDFInterfaceImplementation::AddParam(bool &bData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(bData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(Matrix<std::complex<float> > &cComplexMatrixData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cComplexMatrixData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(Matrix<std::complex<double> > &cComplexMatrixData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cComplexMatrixData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(Matrix<int> &cIntMatrixData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cIntMatrixData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(Matrix<float> &cFloatMatrixData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cFloatMatrixData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(Matrix<double> &cDoubleMatrixData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cDoubleMatrixData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(Matrix<bool> &cBoolMatrixData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(cBoolMatrixData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(FixedPointEnums::Sign &sData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(sData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(FixedPointEnums::QuantizationMode &sData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(sData,pcCodeGenName);
    }

    DFParam CDFInterfaceImplementation::AddParam(FixedPointEnums::OverflowMode &sData, const char *pcCodeGenName)
    {
        //添加参数
        return CreateParam(sData,pcCodeGenName);
    }
    //端口
    DFPort CDFInterfaceImplementation::AddInput(int &iData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(iData,pcCodeGenName,true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(int &iData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(iData,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(double &dData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(dData,pcCodeGenName,true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(double &dData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(dData,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(std::complex<double> &ComplexData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(ComplexData,pcCodeGenName,true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(std::complex<double> &ComplexData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(ComplexData,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(int *&iData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(iData,pcCodeGenName,true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(int *&iData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(iData,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(double *&dData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(dData,pcCodeGenName,true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(double *&dData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(dData,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(std::complex<double> *&ComplexData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(ComplexData,pcCodeGenName,true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(std::complex<double> *&ComplexData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(ComplexData,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<int> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<int> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<double> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<double> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<float> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<float> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<bool> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<bool> &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<std::complex<float> > &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<std::complex<float> > &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<std::complex<double> > &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<std::complex<double> > &circularBuffer, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBufferBus &circularBufferBus, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(circularBufferBus,pcCodeGenName,true);
    }


    DFPort CDFInterfaceImplementation::AddOutput(CircularBufferBus &circularBufferBus, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePort(circularBufferBus,pcCodeGenName,false);
    }

    DFPort CDFInterfaceImplementation::AddInput(EnvelopeCircularBuffer &envelopeData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(envelopeData, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(EnvelopeCircularBuffer &envelopeData, const char *pcCodeGenName)
    {
        //添加端口
        return CreatePortWithSystemVueBuffer(envelopeData, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(IntMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(IntMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(DoubleMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(DoubleMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(FloatMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(FloatMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(BoolMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(BoolMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(FComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(FComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(DComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(DComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(EnvelopeMatrixCircularBuffer &envelopeData, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(envelopeData, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(EnvelopeMatrixCircularBuffer &envelopeData, const char *pcCodeGenName)
    {
        return CreatePortWithSystemVueBuffer(envelopeData, pcCodeGenName, false);
    }



    // void CDFInterfaceImplementation::RegisterSimulationControl(const SinkControl &sinkControl, const char *pcCodeGenName)
    // {
    //     if(pcCodeGenName)
    //         m_sinkControls[pcCodeGenName] = sinkControl;
    // }

    // void CDFInterfaceImplementation::RegisterSimulationControl(const DynamicControl &dynamicControl, const char *pcCodeGenName)
    // {
    //     if(pcCodeGenName)
    //         m_dynamicControls[pcCodeGenName] = dynamicControl;
    // }

    const char *CDFInterfaceImplementation::GetLastError() const
    {
        //获取最近的错误信息
        return m_lastError.c_str();
    }

    const std::vector<DFParam> &CDFInterfaceImplementation::GetParams() const
    {
        //获取参数
        return m_params;
    }

    const std::vector<DFPort> &CDFInterfaceImplementation::GetInputPorts() const
    {
        //获取输入端口
        return m_inputPorts;
    }

    const std::vector<DFPort> &CDFInterfaceImplementation::GetOutputPorts() const
    {
        //获取输出端口
        return m_outputPorts;
    }

    const std::string &CDFInterfaceImplementation::GetModelName() const
    {
        //获取模型名称
        return m_modelName;
    }

    const std::string &CDFInterfaceImplementation::GetModelDescription() const
    {
        //获取模型描述
        return m_modelDescription;
    }

    bool CDFInterfaceImplementation::IsPartGenerationEnabled() const
    {
        //设置部分生产
        return m_bPartGenerationEnabled;
    }

    bool CDFInterfaceImplementation::IsModelGenerationEnabled() const
    {
        //设置模型生成
        return m_bModelGenerationEnabled;
    }

    bool CDFInterfaceImplementation::IsCppCodeGenerationEnabled() const
    {
        //设置C++代码生成
        return m_bCppCodeGenerationEnabled;
    }


    const CDFInterfaceImplementation::SystemVueBufferInfo *
    CDFInterfaceImplementation::GetPortBufferInfo(const std::string &portName) const
    {
        //获取端口buffer
        auto it = m_systemVueBufferMap.find(portName);
        if (it != m_systemVueBufferMap.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::vector<CircularBufferBase *>
    CDFInterfaceImplementation::GetAllSystemVueBuffers() const
    {
        //获取buffer容器
        std::vector<CircularBufferBase*> result;
        for (const auto& pair : m_systemVueBufferMap) {
            if (pair.second.circularBuffer) {
                result.push_back(pair.second.circularBuffer);
            }
        }
        return result;
    }

    std::vector<std::pair<std::string, CircularBufferBase *> >
    CDFInterfaceImplementation::GetAllOutputBuffers() const
    {
        //获取输出buffer容器
        std::vector<std::pair<std::string, CircularBufferBase*>> result;
        for (const auto& pair : m_systemVueBufferMap) {
            if (!pair.second.isInput && pair.second.circularBuffer) {
                result.emplace_back(pair.first, pair.second.circularBuffer);
            }
        }
        return result;
    }

    std::vector<std::pair<std::string, CircularBufferBase *> >
    CDFInterfaceImplementation::GetAllInputBuffers() const
    {
        //获取输入buffer容器
        std::vector<std::pair<std::string, CircularBufferBase*>> result;
        for (const auto& pair : m_systemVueBufferMap) {
            if (pair.second.isInput && pair.second.circularBuffer) {
                result.emplace_back(pair.first, pair.second.circularBuffer);
            }
        }
        return result;
    }

    // FixedPoint implementations
    DFParam CDFInterfaceImplementation::AddParam(FixedPointParameters &fxParam, const char *pcCodeGenName)
    {
        // 添加定点数参数
        if (!pcCodeGenName) {
            m_lastError = "Invalid code generation name";
            return DFParam(nullptr);
        }

        try {
            CDFParamImplementation* paramImpl = new CDFParamImplementation();
            if (pcCodeGenName) {
                paramImpl->SetName(pcCodeGenName);
            }
            paramImpl->SetParamType("FixedPointParameters");

            if (!m_modelName.empty()) {
                paramImpl->PrependCodeGenName(m_modelName.c_str());
            }

            DFParam dfParam(paramImpl);
            if (m_configManager) {
                dfParam.SetConfigManager(m_configManager);
            }

            m_params.push_back(dfParam);
            return dfParam;
        } catch (const std::exception& e) {
            m_lastError = std::string("Failed to create FixedPointParameters parameter: ") + e.what();
            return DFParam(nullptr);
        }
    }

    DFPort CDFInterfaceImplementation::AddInput(FixedPoint &fxData, const char *pcCodeGenName)
    {
        // 添加定点数输入端口 - 使用 CircularBuffer<FixedPoint> 包装
        CircularBuffer<FixedPoint>* buffer = reinterpret_cast<CircularBuffer<FixedPoint>*>(&fxData);
        return CreatePortWithSystemVueBuffer(*buffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(FixedPoint &fxData, const char *pcCodeGenName)
    {
        // 添加定点数输出端口 - 使用 CircularBuffer<FixedPoint> 包装
        CircularBuffer<FixedPoint>* buffer = reinterpret_cast<CircularBuffer<FixedPoint>*>(&fxData);
        return CreatePortWithSystemVueBuffer(*buffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(CircularBuffer<FixedPoint> &circularBuffer, const char *pcCodeGenName)
    {
        // 添加定点数循环缓冲区输入端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(CircularBuffer<FixedPoint> &circularBuffer, const char *pcCodeGenName)
    {
        // 添加定点数循环缓冲区输出端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }

    DFPort CDFInterfaceImplementation::AddInput(FixedPointMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        // 添加定点数矩阵循环缓冲区输入端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, true);
    }

    DFPort CDFInterfaceImplementation::AddOutput(FixedPointMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
    {
        // 添加定点数矩阵循环缓冲区输出端口
        return CreatePortWithSystemVueBuffer(circularBuffer, pcCodeGenName, false);
    }
