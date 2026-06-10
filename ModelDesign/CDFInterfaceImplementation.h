#ifndef CDFINTERFACEIMPLEMENTATION_H
#define CDFINTERFACEIMPLEMENTATION_H

#include <string>
#include <vector>
#include <complex>


#include "configmanager.h"
#include "SimulationControl.h"
#include "DFParam.h"
#include "Matrix.h"
#include "DFPort.h"
#include "Fixpoint/FixedPointEnums.h"
#include "Fixpoint/FixedPoint.h"
#include "EnvelopeSignal.h"
#include "CDFParamImplementation.h"
#include "CDFPortImplementation.h"

#include "Buffer.h"

#include <QDebug>

namespace SystemVueModelBuilder {
    class CDFInterfaceImplementation
    {
    private:
        //--------------------------------------------------------------
        //模型配置信息
        std::string m_modelName;
        std::string m_defaultModel;
        std::string m_modelDescription;
        std::string m_modelCategory;
        std::string m_modelSymbol;
        std::string m_modelNamespace;
        std::string m_modelCodeGenName;
        std::string m_customUI;
        //文件配置
        std::vector<std::string> m_headerFiles;
        std::vector<std::string> m_sourceFiles;
        //功能标志
        bool m_bPartGenerationEnabled;
        bool m_bModelGenerationEnabled;
        bool m_bCppCodeGenerationEnabled;
        bool m_bHidingIOEnabled;
        bool m_bHidingIOFromParamEnabled;
        // 仿真控制
        std::map<std::string, SinkControl> m_sinkControls;
        std::map<std::string, DynamicControl> m_dynamicControls;
        //错误信息
        std::string m_lastError;
        // 配置管理器
        std::shared_ptr<ConfigManager> m_configManager;
        std::string m_configKey;
        //--------------------------------------------------------------
        // 参数和端口管理
        std::vector<DFParam> m_params;
        std::vector<DFPort> m_inputPorts;
        std::vector<DFPort> m_outputPorts;

        struct SystemVueBufferInfo {
                std::shared_ptr<void> bufferPtr;           // 指向实际缓冲区的智能指针
                CircularBufferBase* circularBuffer;        // 指向CircularBufferBase的指针
                void* modelVariable;                       // 指向模型变量的指针
                size_t bufferSize;                         // 缓冲区大小
                DataType dataType;                         // 数据类型
                bool isInput;                              // 输入/输出端口
                bool isTimed;                              // 是否是TimedCircularBuffer
                bool isBus;                                // 是否是总线类型（暂时不用）

                SystemVueBufferInfo()
                    : circularBuffer(nullptr)
                    , modelVariable(nullptr)
                    , bufferSize(1024)  // 默认1024
                    , dataType(DataType::ANY)
                    , isInput(false)
                    , isTimed(false)
                    , isBus(false) {}
        };

        std::map<std::string, SystemVueBufferInfo> m_systemVueBufferMap;
        //--------------------------------------------------------------
    public:
        //DFInterface的实现类
        CDFInterfaceImplementation();
        explicit CDFInterfaceImplementation(const std::string& configKey);
        ~CDFInterfaceImplementation() = default;

        //模型初始化后的json写入
        //判断是否创建了配置管理器
        bool HasConfigManager() const;
        //设置/获取配置管理器
        void SetConfigManager(std::shared_ptr<ConfigManager> manager);
        std::shared_ptr<ConfigManager> GetConfigManager() const;
        //保存端口配置
        void SavePortConfig(const std::string& portKey, const PortConfig& config);
        //保存参数配置
        void SaveParamConfig(const std::string& paramKey, const ParamConfig& config);
        //加载模型配置
        ModelConfig LoadModelConfig();
        //保存模型配置
        void SaveModelConfig(const ModelConfig& config);

        //统一保存
        bool SaveCompleteConfig(const std::string& filename = "");



        // 设置/获取配置键
        void SetConfigKey(const std::string& configKey);
        const std::string& GetConfigKey() const;
        // 配置管理功能
        bool SaveToConfig(ConfigManager& externalManager);//保存配置
        bool LoadFromConfig();//加载配置
        bool RemoveFromConfig();//删除配置
        bool HasConfig() const;//是否是配置
        // 从ParamConfig转换
        void FromModelConfig(const ModelConfig& config);
        // 转换为ParamConfig
        ModelConfig ToModelConfig() const;



        //模型配置方法
        void AddModel(const char* pcModelName);
        void SetDefaultModel(const char* pcModelName);
        void SetModelDescription(const char* pcDescription);
        void SetModelCategory(const char* pcCategory);
        void SetModelSymbol(const char* pcSymbolName);
        void SetModelName(const char* pcName);
        void SetModelNamespace(const char* pcNamespace);
        void SetModelCodeGenName(const char* pcName);
        void SetCustomUI(const char* pcCustomUIName);

        // 功能控制方法
        void DisablePartGeneration();
        void DisablePartAndModelGeneration();
        void EnablePartAndModelGeneration();
        void DisableCppCodeGeneration();
        void EnableHidingIO();
        void EnableHidingIOFromParam();

        //文件管理
        void AddModelHeaderFile(const char* pcHeaderFile);
        void AddModelSourceFile(const char* pcSourceFile);

        // 仿真控制注册
        void RegisterSimulationControl(const SinkControl& sinkControl, const char *pcCodeGenName);
        void RegisterSimulationControl(const DynamicControl& dynamicControl, const char *pcCodeGenName);


        // 参数添加方法
        DFParam AddParam(double &dData, const char *pcCodeGenName);
        DFParam AddParam(float &dData, const char *pcCodeGenName);
        DFParam AddParamArray(double *&pdData, int &iSize, const char *pcCodeGenName, const char* pcSizeName);
        DFParam AddParam(int &iData, const char *pcCodeGenName);
        DFParam AddParamArray(int *&piData, int &iSize, const char *pcCodeGenName, const char* pcSizeName);
        DFParam AddParam(std::complex<float> &cComplexData, const char *pcCodeGenName);
        DFParam AddParam(std::complex<double> &cComplexData, const char *pcCodeGenName);
        DFParam AddParamArray(std::complex<double> *&pcComplexData, int &iSize, const char *pcCodeGenName, const char* pcSizeName);
        DFParam AddParam(char *&sData, const char *pcCodeGenName);
        template <typename T> DFParam AddParamEnum(T &eEnumData, const char *pcCodeGenName, const char *pcEnumType)
        {
            if(eEnumData) {}
            // Only Enum of size int are supported
            //判断是否属于枚举类型
            bool enumHasCorrectSize = ( sizeof(T) <= sizeof(int) ) || ( sizeof(T) == sizeof(long) ) || ( sizeof(T) == sizeof(long long) );

            if(enumHasCorrectSize)
            {
                // int *data;
                // data = (int *)&eEnumData;
                // return AddParamEnum(data,pcCodeGenName,pcEnumType);
                try {
                    CDFParamImplementation* paramImpl = new CDFParamImplementation();

                    //设置名称
                    if(pcCodeGenName) {
                        paramImpl->SetName(pcCodeGenName);
                    }
                    //设置枚举参数类型
                    paramImpl->SetParamType(pcEnumType);
                    if(!paramImpl->Validate()) {
                        delete paramImpl;
                        m_lastError = "Enum parameter validation failed";
                         return DFParam(NULL);
                    }

                    DFParam dfParam(paramImpl);
                    //保存参数的配置
                    if(m_configManager) {
                        dfParam.SetConfigManager(m_configManager);
                    }
                    m_params.push_back(dfParam);
                    return dfParam;
                } catch (const std::exception& e) {
                    m_lastError = std::string("Failed to create enum parameter: ") + e.what();
                    return DFParam(nullptr);
                }
            }
            else
            {
                //未知枚举类型参数，保存为int类型
                qDebug() << "WARNING: Enum type size" << sizeof(T) << "may not be compatible with int";
                try {
                    CDFParamImplementation* paramImpl = new CDFParamImplementation();

                    if (pcCodeGenName) {
                        paramImpl->SetName(pcCodeGenName);
                    }

                    paramImpl->SetParamType("int");

                    if (!m_modelName.empty()) {
                        paramImpl->PrependCodeGenName(m_modelName.c_str());
                    }

                    DFParam dfParam(paramImpl);
                    if (m_configManager) {
                        dfParam.SetConfigManager(m_configManager);
                    }

                    m_params.push_back(dfParam);
                    return dfParam;

                } catch (...) {
                    m_lastError = "Cannot create enum parameter due to type size incompatibility";
                    return DFParam(nullptr);
                }
            }

        }
        //添加特殊枚举类型的参数
        DFParam AddParam(QueryEnum &qData, const char *pcCodeGenName);
        DFParam AddParam(BooleanEnum &bData, const char *pcCodeGenName);
        DFParam AddParam(SwitchEnum &sData, const char *pcCodeGenName);
        //添加文件参数
        DFParam AddParamFile(const char* pcName, const char *pcDescription, char *&sData, const char* pcValue = "");
        DFParam AddParam(bool &bData, const char *pcCodeGenName);
        //添加矩阵类型的参数
        DFParam AddParam(Matrix<std::complex<float>> &cComplexMatrixData, const char *pcCodeGenName);
        DFParam AddParam(Matrix<std::complex<double>> &cComplexMatrixData, const char *pcCodeGenName);
        DFParam AddParam(Matrix<int> &cIntMatrixData, const char *pcCodeGenName);
        DFParam AddParam(Matrix<float> &cFloatMatrixData, const char *pcCodeGenName);
        DFParam AddParam(Matrix<double> &cDoubleMatrixData, const char *pcCodeGenName);
        DFParam AddParam(Matrix<bool> &cBoolMatrixData, const char *pcCodeGenName);
        DFParam AddParam(FixedPointEnums::Sign &sData, const char *pcCodeGenName);
        DFParam AddParam(FixedPointEnums::QuantizationMode &sData, const char *pcCodeGenName);
        DFParam AddParam(FixedPointEnums::OverflowMode &sData, const char *pcCodeGenName);

        // 端口添加方法
        DFPort AddInput(int &iData, const char *pcCodeGenName);
        DFPort AddOutput(int &iData, const char *pcCodeGenName);
        DFPort AddInput(double &dData, const char *pcCodeGenName);
        DFPort AddOutput(double &dData, const char *pcCodeGenName);
        DFPort AddInput(std::complex<double> &ComplexData, const char *pcCodeGenName);
        DFPort AddOutput(std::complex<double> &ComplexData, const char *pcCodeGenName);
        DFPort AddInput(int *&iData, const char *pcCodeGenName);
        DFPort AddOutput(int *&iData, const char *pcCodeGenName);
        DFPort AddInput(double *&dData, const char *pcCodeGenName);
        DFPort AddOutput(double *&dData, const char *pcCodeGenName);
        DFPort AddInput(std::complex<double> *&ComplexData, const char *pcCodeGenName);
        DFPort AddOutput(std::complex<double> *&ComplexData, const char *pcCodeGenName);
//        DFPort AddInput(CircularBufferBase& circularBuffer, const char *pcCodeGenName);
//        DFPort AddOutput(CircularBufferBase& circularBuffer, const char *pcCodeGenName);

        //Circular Buffer
        //int
        DFPort AddInput(CircularBuffer<int>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<int>& circularBuffer, const char *pcCodeGenName);
        //double
        DFPort AddInput(CircularBuffer<double>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<double>& circularBuffer, const char *pcCodeGenName);
        //float
        DFPort AddInput(CircularBuffer<float>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<float>& circularBuffer, const char *pcCodeGenName);
        //bool
        DFPort AddInput(CircularBuffer<bool>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<bool>& circularBuffer, const char *pcCodeGenName);
        //complex float
        DFPort AddInput(CircularBuffer<std::complex<float>>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<std::complex<float>>& circularBuffer, const char *pcCodeGenName);
        //complex double
        DFPort AddInput(CircularBuffer<std::complex<double>>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<std::complex<double>>& circularBuffer, const char *pcCodeGenName);

        DFPort AddInput(CircularBufferBus& circularBufferBus, const char *pcCodeGenName);
        DFPort AddOutput(CircularBufferBus& circularBufferBus, const char *pcCodeGenName);
        //envelope
        DFPort AddInput( EnvelopeCircularBuffer& envelopeData, const char *pcCodeGenName);
        DFPort AddOutput( EnvelopeCircularBuffer& envelopeData, const char *pcCodeGenName);

        //Matrix Circular Buffer
        //int
        DFPort AddInput(IntMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(IntMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        //double
        DFPort AddInput(DoubleMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(DoubleMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        //float
        DFPort AddInput(FloatMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(FloatMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        //bool
        DFPort AddInput(BoolMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(BoolMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        //complex float
        DFPort AddInput(FComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(FComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        //complex double
        DFPort AddInput(DComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(DComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        //envelope
        DFPort AddInput(EnvelopeMatrixCircularBuffer& envelopeData, const char* pcCodeGenName);
        DFPort AddOutput(EnvelopeMatrixCircularBuffer& envelopeData, const char* pcCodeGenName);

        //FixedPoint
        DFParam AddParam(FixedPointParameters &fxParam, const char *pcCodeGenName);
        DFPort AddInput(FixedPoint &fxData, const char *pcCodeGenName);
        DFPort AddOutput(FixedPoint &fxData, const char *pcCodeGenName);
        DFPort AddInput(CircularBuffer<FixedPoint> &circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(CircularBuffer<FixedPoint> &circularBuffer, const char *pcCodeGenName);
        DFPort AddInput(FixedPointMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput(FixedPointMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName);



        // 获取方法
        const char* GetLastError() const;
        const std::vector<DFParam>& GetParams() const;
        const std::vector<DFPort>& GetInputPorts() const;
        const std::vector<DFPort>& GetOutputPorts() const;
        const std::string& GetModelName() const;
        const std::string& GetModelDescription() const;

        // 状态检查方法
        bool IsPartGenerationEnabled() const;
        bool IsModelGenerationEnabled() const;
        bool IsCppCodeGenerationEnabled() const;

        //--------------------------------------------------------------
        // 辅助函数：获取缓冲区类型字符串
        template<typename T>
        std::string GetBufferTypeString(const T& buffer) const
        {
            std::ignore = buffer;
            if constexpr (std::is_same_v<T, DoubleCircularBuffer>) {
                return "double";
            }
            else if constexpr (std::is_same_v<T, DComplexCircularBuffer>) {
                return "complex_double";
            }
            else if constexpr (std::is_same_v<T, IntCircularBuffer>) {
                return "int";
            }
            else if constexpr (std::is_same_v<T, FloatCircularBuffer>) {
                return "float";
            }
            else if constexpr (std::is_same_v<T, BoolCircularBuffer>) {
                return "bool";
            }
            else if constexpr (std::is_same_v<T, TimedCircularBuffer<double>>) {
                return "timed_double";
            }
            else if constexpr (std::is_same_v<T, TimedCircularBuffer<int>>) {
                return "timed_int";
            }
            else if constexpr (std::is_same_v<T, TimedCircularBuffer<std::complex<double>>>) {
                return "timed_complex_double";
            }
            else if constexpr (std::is_same_v<T, EnvelopeCircularBuffer>) {
                return "envelope";
            }
            else if constexpr (std::is_same_v<T, CircularBuffer<FixedPoint>>) {
                return "fixedpoint";
            }
            return "unknown";
        }

        //创建端口的缓冲区
        template<typename T>
        DFPort CreatePortWithSystemVueBuffer(T& buffer, const char* pcCodeGenName, bool isInput)
        {
            if (!pcCodeGenName) {
                m_lastError = "Invalid code generation name";
                return DFPort(nullptr);
            }

            std::string portName = pcCodeGenName;

            try {
                // 1. 创建端口配置
                DFPort port = CreatePort(buffer, pcCodeGenName, isInput);
                if (!port.GetImplementation()) {
                    return port;
                }

                // 2. 创建并初始化SystemVue缓冲区
                if (!CreateAndInitializeBuffer(buffer, portName, isInput)) {
                    qDebug() << "create port buffer error!";
                    return port;  // 创建失败，返回基本端口
                }

                // 3. 保存配置
                if (m_configManager) {
                    PortConfig portConfig;
                    portConfig.name = portName;
                    portConfig.dataType = GetBufferTypeString(buffer);
                    portConfig.putType = isInput ? "in" : "out";
                    m_configManager->SavePortConfig(portName, portConfig);
                }

                qDebug() << "Created SystemVue buffer for port: " << QString::fromStdString(portName)
                          << " (size: 1024, type: " << QString::fromStdString(GetBufferTypeString(buffer))
                          << ", " << (isInput ? "input" : "output") << ")";

                return port;

            } catch (const std::exception& e) {
                m_lastError = std::string("Failed to create port '") + portName + "': " + e.what();
                return DFPort(nullptr);
            }
        }
        // 获取端口的SystemVue缓冲区
        CircularBufferBase* GetPortSystemVueBuffer(const std::string& portName)
        {
            auto it = m_systemVueBufferMap.find(portName);
            if (it != m_systemVueBufferMap.end()) {
                return it->second.circularBuffer;
            }
            return nullptr;
        }

        // 获取端口缓冲区信息
        const SystemVueBufferInfo* GetPortBufferInfo(const std::string& portName) const;

        // 获取所有端口的缓冲区
        std::vector<CircularBufferBase*> GetAllSystemVueBuffers() const;

        // 获取所有输出端口的缓冲区（用于Block的Connect方法）
        std::vector<std::pair<std::string, CircularBufferBase*>> GetAllOutputBuffers() const;

        // 获取所有输入端口的缓冲区（用于Block的Connect方法）
        std::vector<std::pair<std::string, CircularBufferBase*>> GetAllInputBuffers() const;
        //--------------------------------------------------------------
    private:
        // 内部辅助方法
        template<typename T>inline DFParam CreateParam(T& pData, const char* pcCodeGenName,
                                   const char* pcEnumType = nullptr, int iSize = 0)
        {
            if(!pcCodeGenName) {
                m_lastError = "Invalid parameter data or code generation name";
                return DFParam(nullptr);
            }
            try {
                CDFParamImplementation* paramImpl = new CDFParamImplementation();

                //设置参数名称
                paramImpl->SetName(pcCodeGenName);
                //设置参数的数据类型
                if (pcEnumType != nullptr) {
                    // 枚举类型参数
                    paramImpl->SetParamType(pcEnumType);
                    qDebug() << "Creating enum parameter:" << pcCodeGenName << "with type:" << pcEnumType;
                }
                else if(iSize > 0) {
                    // 数组类型参数
                    std::string arrayType = GetTypeName<T>() + "[]";
                    paramImpl->SetParamType(arrayType.c_str());
                    // 处理数组大小信息
                }
                else {
                    // 标量类型参数
                    std::string typeName = GetTypeName<T>();
                    paramImpl->SetParamType(typeName.c_str());
                    // 设置默认值
                    std::string defaultValue = ConvertValueToString(pData);
                    paramImpl->SetDefaultValue(defaultValue.c_str());
                }
                // 设置代码生成路径
                if (!m_modelName.empty()) {
                    paramImpl->PrependCodeGenName(m_modelName.c_str());
                }
                //验证参数配置
                if (!paramImpl->Validate()) {
                    delete paramImpl;
                    m_lastError = "Parameter validation failed";
                    return DFParam(nullptr);
                }

                // 创建DFParam对象并添加到参数列表
                DFParam dfParam(paramImpl);
                if (m_configManager) {
                    dfParam.SetConfigManager(m_configManager);
                }
                //保存配置
                if(m_configManager) {
                    ParamConfig paramConfig;
                    paramConfig.name = pcCodeGenName;
                    paramConfig.dataType = pcEnumType ? pcEnumType : GetTypeName<T>();
                    // 枚举参数，设置默认值
                    if (pcEnumType && !paramImpl->GetDefaultValue().empty()) {
                        paramConfig.defaultValue = paramImpl->GetDefaultValue();
                    }

                    // 保存枚举信息
                    auto enumerations = paramImpl->GetEnumeration();
                    for (const auto& enumPair : enumerations) {
                        paramConfig.enumValues.push_back(enumPair.first);
                    }

                    m_configManager->SaveParamConfig(pcCodeGenName, paramConfig);
                }
                m_params.push_back(dfParam);
                return dfParam;
            } catch (const std::exception& e) {
                m_lastError = std::string("Failed to create parameter: ") + e.what();
                return DFParam(nullptr);
            }
        }

        //创建端口方法，用于不属于CircularBuffer的端口，以及bus类型的端口
        template<typename T>inline DFPort CreatePort(T& pData, const char* pcCodeGenName,
                                 bool isInput, bool isArray = false)
        {
            std::ignore = pData;
            if(!pcCodeGenName) {
                m_lastError = "Invalid parameter data or code generation name";
                return DFPort(nullptr);
            }
            try {
                CDFPortImplementation* portImpl = new CDFPortImplementation();

                //设置端口名称
                portImpl->SetName(pcCodeGenName);

                //设置端口的输入输出类型
                std::string putType;
                if(isInput) {
                    putType = "in";
                }
                else {
                    putType = "out";
                }

                if(isArray) {
                    putType += "[]";

                }
                portImpl->SetPutType(putType.c_str());

                //设置端口的参数类型
                std::string dataType = GetTypeName<T>();
                portImpl->SetDataType(dataType.c_str());

                // 设置描述（基于端口类型和方向）
                std::string description;
                if (isInput) {
                    description = "Input port for " + dataType;
                    if (isArray) description += " array";
                    description += " data. Set by simulator and accessible in run method.";
                } else {
                    description = "Output port for " + dataType;
                    if (isArray) description += " array";
                    description += " data. Set in run method and read by simulator.";
                }
                portImpl->SetDescription(description.c_str());

                // 设置代码生成路径
                if (!m_modelName.empty()) {
                    portImpl->PrependCodeGenName(m_modelName.c_str());
                }

                //验证端口配置
                if (!portImpl->Validate()) {
                    delete portImpl;
                    m_lastError = "Port validation failed";
                    return DFPort(nullptr);
                }

                // 创建并返回DFPort包装对象
                DFPort dfPort(portImpl);
                if (isInput) {
                    m_inputPorts.push_back(dfPort);
                } else {
                    m_outputPorts.push_back(dfPort);
                }
                //保存配置
                if(m_configManager) {
                    dfPort.SetConfigManager(m_configManager);
                    PortConfig portConfig;
                    portConfig.name = pcCodeGenName;
                    portConfig.dataType = GetTypeName<T>();
                    portConfig.putType = isInput ? "in" : "out";

                    m_configManager->SavePortConfig(pcCodeGenName, portConfig);
                }

                return dfPort;
            } catch (const std::exception& e) {
                m_lastError = std::string("Failed to create parameter: ") + e.what();
                return DFPort(nullptr);
            }
        }

        //获取类型名称
        template<typename T> inline std::string GetTypeName()
        {
            if constexpr (std::is_same_v<T,double>) return "double";
            else if constexpr (std::is_same_v<T,float>) return "float";
            else if constexpr (std::is_same_v<T,int>) return "int";
            else if constexpr (std::is_same_v<T,bool>) return "bool";
            else if constexpr (std::is_same_v<T,char*>) return "string";
            else if constexpr (std::is_same_v<T,std::complex<float>>) return "std::complex<float>";
            else if constexpr (std::is_same_v<T,std::complex<double>>) return "std::complex<double>";
            else if constexpr (std::is_same_v<T,QueryEnum>) return "QueryEnum";
            else if constexpr (std::is_same_v<T,BooleanEnum>) return "BooleanEnum";
            else if constexpr (std::is_same_v<T,SwitchEnum>) return "SwitchEnum";
            else if constexpr (std::is_same_v<T, int*>) return "int";
            else if constexpr (std::is_same_v<T, double*>) return "double";
            else if constexpr (std::is_same_v<T, std::complex<double>*>) return "complex<double>";
            else if constexpr (std::is_same_v<T, CircularBufferBase>) return "CircularBuffer";
            else if constexpr (std::is_same_v<T, CircularBufferBus>) return "CircularBufferBus";
            else if constexpr (std::is_same_v<T, EnvelopeSignal>) return "EnvelopeSignal";
            else
                return typeid(T).name();

        }
        //转换数据类型值为string类型
        template<typename T> inline std::string ConvertValueToString(T& Data)
        {
            if constexpr (std::is_same_v<T, std::string>)
                return Data;
            else if constexpr (std::is_same_v<T, int>)
                return std::to_string(Data);
            else if constexpr (std::is_same_v<T, double>)
                return std::to_string(Data);
            else if constexpr (std::is_same_v<T, float>)
                return std::to_string(Data);
            else if constexpr (std::is_same_v<T, bool>)
                return (Data) ? "true" : "false";
            else if constexpr (std::is_same_v<T, char*>)
                return Data ? std::string(Data) : "";
            else if constexpr (std::is_same_v<T,std::complex<float>>)
                return "(" + std::to_string(Data.real()) + "," + std::to_string(Data.imag()) + ")";
            else if constexpr (std::is_same_v<T,std::complex<double>>)
                return "(" + std::to_string(Data.real()) + "," + std::to_string(Data.imag()) + ")";
            else if constexpr (std::is_same_v<T,QueryEnum> ||
                               std::is_same_v<T,BooleanEnum> ||
                               std::is_same_v<T,SwitchEnum>)
                return EnumToString(&Data);
            else if constexpr (std::is_same_v<T,FixedPointEnums::Sign> ||
                               std::is_same_v<T,FixedPointEnums::QuantizationMode> ||
                               std::is_same_v<T,FixedPointEnums::OverflowMode>)
                return EnumToString(&Data);
            else if constexpr (std::is_same_v<T,Matrix<std::complex<float>>> ||
                               std::is_same_v<T,Matrix<std::complex<double>>> ||
                               std::is_same_v<T,Matrix<int>> ||
                               std::is_same_v<T,Matrix<float>> ||
                               std::is_same_v<T,Matrix<double>> ||
                               std::is_same_v<T,Matrix<bool>>)
                return MatrixToString(&Data);
            else {
                return typeid(T).name();
            }
        }
        //转换特殊枚举类型值为string类型
        template<typename T> inline std::string EnumToString(T* pData)
        {
            if(!pData) return "unknown";
            int enumValue = static_cast<int>(*pData);
            //转换QueryEnum枚举
            if constexpr (std::is_same_v<T,QueryEnum>) {
                switch(enumValue)
                {
                //查询 枚举
                case 0: return "QUERY_NO";
                case 1: return "QUERY_YES";
                default: return "QueryEnum_" + std::to_string(enumValue);
                }
            }
            else if constexpr (std::is_same_v<T,BooleanEnum>) {
                switch(enumValue)
                {
                //判断 枚举
                case 0: return "BOOLEAN_FALSE";
                case 1: return "BOOLEAN_TURE";
                default: return "BooleanEnum_" + std::to_string(enumValue);
                }
            }
            else if constexpr (std::is_same_v<T,SwitchEnum>) {
                switch(enumValue)
                {
                //转换 枚举
                case 0: return "SWITCH_OFF";
                case 1: return "SWITCH_ON";
                default: return "SwitchEnum_" + std::to_string(enumValue);
                }
            }
            else if constexpr (std::is_same_v<T,FixedPointEnums::Sign>) {
                switch(enumValue)
                {
                //符号编码 枚举
                case 0: return "UNSIGNED";
                case 1: return "TWOS_COMPLEMENT";
                default: return "Sign_" + std::to_string(enumValue);
                }
            }
            else if constexpr (std::is_same_v<T,FixedPointEnums::QuantizationMode>) {
                switch(enumValue)
                {
                //量化模式 枚举
                case 0: return "ROUND";
                case 1: return "ROUND_ZERO";
                case 2: return "ROUND_MINUS_INFINITY";
                case 3: return "ROUND_INFINITY";
                case 4: return "ROUND_CONVERGENT";
                case 5: return "TRUNCATE";
                case 6: return "TRUNCATE_ZERO";
                default: return "QuantizationMode_" + std::to_string(enumValue);
                }
            }
            else if constexpr (std::is_same_v<T,FixedPointEnums::OverflowMode>) {
                switch(enumValue)
                {
                //溢出模式 枚举
                case 0: return "SATURATE";
                case 1: return "SATURATE_ZERO";
                case 2: return "SATURATE_SYMMETRICAL";
                case 3: return "WRAP";
                case 4: return "WRAP_SIGN_MAGNITUDE";
                default: return "OverflowMode_" + std::to_string(enumValue);
                }
            }
            else {
                return std::to_string(enumValue);
            }
        }
        //转换矩阵类型为string类型
        template<typename T> inline std::string MatrixToString(Matrix<T>* pMatrix)
        {
            if(!pMatrix || pMatrix->IsEmpty())
                return "Matrix[empty]";
            std::string result = "Matrix[";

            if(pMatrix->IsMatrix()) {
                //2D矩阵
                result += std::to_string(pMatrix->NumRows()) + "x" + std::to_string(pMatrix->NumColumns());
            }
            else {
                //多维数组
                result += "dims:";
                for(size_t i = 0; i < pMatrix->NumDimensions(); i++) {
                    if(i > 0)
                        result += "x";
                    result += std::to_string(pMatrix->Size(i));
                }
            }
            result += "]";

            //当矩阵最大元素数超过6时，用于区分普通多参数和复数，bool类型
            if(pMatrix->NumElements() >= 6) {
                result += "{";
                for(size_t i = 0; i < pMatrix->NumElements(); i++) {
                    if(i > 0) result += ",";
                    if constexpr (std::is_same_v<T,std::complex<float>> ||
                                  std::is_same_v<T,std::complex<double>>) {
                        T value = (*pMatrix)(i);
                        result += "(" + std::to_string(value.real()) + "," + std::to_string(value.imag()) + ")";
                    }
                    else if constexpr (std::is_same_v<T,bool>) {
                        result += (*pMatrix)(i) ? "true" : "false";
                    }
                    else {
                        result += std::to_string((*pMatrix)(i));
                    }
                }
                result += "}";
            }
            return result;
        }


        // 创建和初始化SystemVue缓冲区的方法
        template<typename T>
        bool CreateAndInitializeBuffer(T& modelVariable,
                                       const std::string& portName,
                                       bool isInput)
        {
            try {
                SystemVueBufferInfo info;
                info.modelVariable = &modelVariable;
                info.isInput = isInput;
                info.bufferSize = 1024;  // 初始大小1024

                // 根据类型设置dataType和创建具体的缓冲区
                //DoubleCircularBuffer
                if constexpr (std::is_same_v<T, DoubleCircularBuffer>) {
                    info.dataType = DataType::CIRCULAR_BUFFER_DOUBLE;
                    auto buffer = std::make_shared<CircularBuffer<double>>();
                    InitializeCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                //DComplexCircularBuffer
                else if constexpr (std::is_same_v<T, DComplexCircularBuffer>) {
                    info.dataType = DataType::CIRCULAR_BUFFER_DCOMPLEX;
                    auto buffer = std::make_shared<CircularBuffer<std::complex<double>>>();
                    InitializeCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                //IntCircularBuffer
                else if constexpr (std::is_same_v<T, IntCircularBuffer>) {
                    info.dataType = DataType::CIRCULAR_BUFFER_INT;
                    auto buffer = std::make_shared<CircularBuffer<int>>();
                    InitializeCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                //FloatCircularBuffer
                else if constexpr (std::is_same_v<T, FloatCircularBuffer>) {
                    info.dataType = DataType::CIRCULAR_BUFFER_FLOAT;
                    auto buffer = std::make_shared<CircularBuffer<float>>();
                    InitializeCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                //BoolCircularBuffer
                else if constexpr (std::is_same_v<T, BoolCircularBuffer>) {
                    info.dataType = DataType::CIRCULAR_BUFFER_BOOL;
                    auto buffer = std::make_shared<CircularBuffer<bool>>();
                    InitializeCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // TimedCircularBuffer<double>
                else if constexpr (std::is_same_v<T, TimedCircularBuffer<double>>) {
                    info.dataType = DataType::TIMED_DOUBLE;
                    info.isTimed = true;
                    auto buffer = std::make_shared<TimedCircularBuffer<double>>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // TimedCircularBuffer<int>
                else if constexpr (std::is_same_v<T, TimedCircularBuffer<int>>) {
                    info.dataType = DataType::TIMED_INT;
                    info.isTimed = true;
                    auto buffer = std::make_shared<TimedCircularBuffer<int>>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // TimedCircularBuffer<float>
                else if constexpr (std::is_same_v<T, TimedCircularBuffer<float>>) {
                    info.dataType = DataType::TIMED_INT;
                    info.isTimed = true;
                    auto buffer = std::make_shared<TimedCircularBuffer<float>>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // TimedCircularBuffer<bool>
                else if constexpr (std::is_same_v<T, TimedCircularBuffer<bool>>) {
                    info.dataType = DataType::TIMED_INT;
                    info.isTimed = true;
                    auto buffer = std::make_shared<TimedCircularBuffer<bool>>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // TimedCircularBuffer<std::complex<double>>
                else if constexpr (std::is_same_v<T, TimedCircularBuffer<std::complex<double>>>) {
                    info.dataType = DataType::TIMED_DCOMPLEX;
                    info.isTimed = true;
                    auto buffer = std::make_shared<TimedCircularBuffer<std::complex<double>>>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // TimedCircularBuffer<std::complex<float>>
                else if constexpr (std::is_same_v<T, TimedCircularBuffer<std::complex<float>>>) {
                    info.dataType = DataType::TIMED_DCOMPLEX;
                    info.isTimed = true;
                    auto buffer = std::make_shared<TimedCircularBuffer<std::complex<float>>>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                // EnvelopeCircularBuffer
                else if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeCircularBuffer>) {
                    info.dataType = DataType::ENVELOPE_SIGNAL;
                    info.isTimed = true;
                    auto buffer = std::make_shared<SystemVueModelBuilder::EnvelopeCircularBuffer>();
                    InitializeTimedCircularBuffer(*buffer, 1024);
                    info.bufferPtr = buffer;
                    info.circularBuffer = buffer.get();
                }
                else {
                    // 未知类型，不创建缓冲区
                    qDebug() << "Unkown type";
                    m_lastError = "Unsupported buffer type for port: " + portName;
                    return false;
                }

                // 存储到map中
                m_systemVueBufferMap[portName] = info;

                // 将创建的缓冲区关联到模型变量
                AssociateBufferToModelVariable(modelVariable, info);

                return true;

            } catch (const std::exception& e) {
                m_lastError = std::string("Failed to create buffer for port ") +
                              portName + ": " + e.what();
                return false;
            }
        }

        // 初始化Circularbuffer方法
        template<typename T>
        void InitializeCircularBuffer(CircularBuffer<T>& buffer, size_t size)
        {
            void* memory = buffer.AllocateMemory(size);
            if (memory) {
                buffer.SetBuffer(memory, size);
                // 初始化为0
                buffer.Initialize();
            }
        }

        // 初始化TimedCircularBuffer方法
        template<typename T>
        void InitializeTimedCircularBuffer(TimedCircularBuffer<T>& buffer, size_t size)
        {
            void* memory = buffer.AllocateMemory(size);
            if (memory) {
                buffer.SetBuffer(memory, size);
                // 初始化为0
                buffer.Initialize();
            }
        }
        // 初始化EnvelopeCircularBuffer方法
        void InitializeTimedCircularBuffer(SystemVueModelBuilder::EnvelopeCircularBuffer& buffer, size_t size)
        {
            void* memory = buffer.AllocateMemory(size);
            if (memory) {
                buffer.SetBuffer(memory, size);
                // 初始化为0
                buffer.Initialize();
            }
        }

        //关联Buffer到模型变量中
        template<typename T>
        void AssociateBufferToModelVariable(T& modelVariable, const SystemVueBufferInfo& info)
        {
            // 这里需要将info.circularBuffer的内存设置到modelVariable中
            // 注意：实际上替换了模型变量的内部缓冲区

            if (info.circularBuffer && info.circularBuffer->GetBufferMemory()) {
                // 获取创建的缓冲区内存
                void* createdMemory = info.circularBuffer->GetBufferMemory();
                size_t createdSize = info.circularBuffer->GetSize();

                // 将模型变量的缓冲区指向我们创建的内存
                // 这需要模型变量支持SetBuffer方法
                modelVariable.SetBuffer(createdMemory, createdSize);

            }
        }

    };


    }
#endif // CDFINTERFACEIMPLEMENTATION_H
