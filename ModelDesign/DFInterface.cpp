#include "DFInterface.h"
#include "CDFInterfaceImplementation.h"

SystemVueModelBuilder::DFInterface::DFInterface()
    : m_cImplementation(new CDFInterfaceImplementation())
{
    // 默认构造函数，创建默认的CDFInterfaceImplementation实例
}

SystemVueModelBuilder::DFInterface::DFInterface(const std::string &configKey)
    : m_cImplementation(new CDFInterfaceImplementation())
{
    // 带配置键的构造函数
    m_cImplementation->SetConfigKey(configKey);

    // 创建配置管理器并关联到实现对象
    auto configManager = std::make_shared<ConfigManager>(configKey + "_config.json");
    m_cImplementation->SetConfigManager(configManager);
}

SystemVueModelBuilder::DFInterface::~DFInterface()
{
    // 析构函数，释放实现对象内存
    delete m_cImplementation;
}

void SystemVueModelBuilder::DFInterface::AddModel(const char *pcModelName)
{
    // 添加模型名称
    if(m_cImplementation && pcModelName) {
        m_cImplementation->AddModel(pcModelName);
    }
}

void SystemVueModelBuilder::DFInterface::SetDefaultModel(const char *pcModelName)
{
    // 设置默认模型名称
    if(m_cImplementation && pcModelName) {
        m_cImplementation->SetModelName(pcModelName);
    }
}

void SystemVueModelBuilder::DFInterface::SetModelDescription(const char *pcDescription)
{
    // 设置模型描述
    if(m_cImplementation && pcDescription) {
        m_cImplementation->SetModelDescription(pcDescription);
    }
}

void SystemVueModelBuilder::DFInterface::SetModelCategory(const char *pcCategory)
{
    // 设置模型类别/分类
    if(m_cImplementation && pcCategory) {
        m_cImplementation->SetModelCategory(pcCategory);
    }
}

void SystemVueModelBuilder::DFInterface::SetModelSymbol(const char *pcSymbolName)
{
    // 设置模型符号名称
    if(m_cImplementation && pcSymbolName) {
        m_cImplementation->SetModelSymbol(pcSymbolName);
    }
}

void SystemVueModelBuilder::DFInterface::SetModelName(const char *pcName)
{
    // 设置模型名称
    if(m_cImplementation && pcName) {
        m_cImplementation->SetModelName(pcName);
    }
}

void SystemVueModelBuilder::DFInterface::DisablePartGeneration()
{
    // 禁用部件生成功能
    if(m_cImplementation) {
        m_cImplementation->DisablePartGeneration();
    }
}

void SystemVueModelBuilder::DFInterface::DisablePartAndModelGeneration()
{
    // 禁用部件和模型生成功能
    if(m_cImplementation) {
        m_cImplementation->DisablePartAndModelGeneration();
    }
}

void SystemVueModelBuilder::DFInterface::EnablePartAndModelGeneration()
{
    // 启用部件和模型生成功能
    if(m_cImplementation) {
        m_cImplementation->EnablePartAndModelGeneration();
    }
}

void SystemVueModelBuilder::DFInterface::DisableCppCodeGeneration()
{
    // 禁用C++代码生成功能
    if(m_cImplementation) {
        m_cImplementation->DisableCppCodeGeneration();
    }
}

void SystemVueModelBuilder::DFInterface::EnableHidingIO()
{
    // 启用隐藏输入输出端口
    if(m_cImplementation) {
        m_cImplementation->EnableHidingIO();
    }
}

void SystemVueModelBuilder::DFInterface::EnableHidingIOFromParam()
{
    // 启用从参数隐藏输入输出端口
    if(m_cImplementation) {
        m_cImplementation->EnableHidingIOFromParam();
    }
}

void SystemVueModelBuilder::DFInterface::SetCustomUI(const char* pcCustomUIName)
{
    // 设置自定义用户界面名称
    if(m_cImplementation && pcCustomUIName) {
        m_cImplementation->SetCustomUI(pcCustomUIName);
    }
}

void SystemVueModelBuilder::DFInterface::SetModelNamespace(const char* pcNamespace)
{
    // 设置模型命名空间
    if(m_cImplementation && pcNamespace) {
        m_cImplementation->SetModelNamespace(pcNamespace);
    }
}

void SystemVueModelBuilder::DFInterface::SetModelCodeGenName(const char *pcName)
{
    // 设置模型代码生成名称
    if(m_cImplementation && pcName) {
        m_cImplementation->SetModelCodeGenName(pcName);
    }
}

void SystemVueModelBuilder::DFInterface::RegisterSimulationControl(const SinkControl &sinkControl, const char *pcCodeGenName)
{
    // 注册Sink类型的仿真控制
    if(m_cImplementation && pcCodeGenName) {
        m_cImplementation->RegisterSimulationControl(sinkControl, pcCodeGenName);
    }
}

void SystemVueModelBuilder::DFInterface::RegisterSimulationControl(const DynamicControl& dynamicControl, const char *pcCodeGenName)
{
    // 注册Dynamic类型的仿真控制
    if(m_cImplementation && pcCodeGenName) {
        m_cImplementation->RegisterSimulationControl(dynamicControl, pcCodeGenName);
    }
}

void SystemVueModelBuilder::DFInterface::AddModelHeaderFile(const char* pcHeaderFile)
{
    // 添加模型头文件
    if(m_cImplementation && pcHeaderFile) {
        m_cImplementation->AddModelHeaderFile(pcHeaderFile);
    }
}

void SystemVueModelBuilder::DFInterface::AddModelSourceFile(const char* pcSourceFile)
{
    // 添加模型源文件
    if(m_cImplementation && pcSourceFile) {
        m_cImplementation->AddModelSourceFile(pcSourceFile);
    }
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(double &dData, const char *pcCodeGenName)
{
    // 添加double类型参数
    return m_cImplementation->AddParam(dData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(float &dData, const char *pcCodeGenName)
{
    // 添加float类型参数
    return m_cImplementation->AddParam(dData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParamArray(double *&pdData, int &iSize,const char *pcCodeGenName, const char* pcSizeName)
{
    // 添加double数组类型参数
    return m_cImplementation->AddParamArray(pdData, iSize, pcCodeGenName, pcSizeName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(int &iData,const char *pcCodeGenName)
{
    // 添加int类型参数
    return m_cImplementation->AddParam(iData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParamArray(int *&piData, int &iSize,const char *pcCodeGenName, const char* pcSizeName)
{
    // 添加int数组类型参数
    return m_cImplementation->AddParamArray(piData, iSize, pcCodeGenName, pcSizeName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(std::complex<float> &cComplexData, const char *pcCodeGenName)
{
    // 添加单精度复数类型参数
    return m_cImplementation->AddParam(cComplexData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(std::complex<double> &cComplexData, const char *pcCodeGenName)
{
    // 添加双精度复数类型参数
    return m_cImplementation->AddParam(cComplexData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParamArray(std::complex<double> *&pcComplexData, int &iSize, const char *pcCodeGenName, const char *pcSizeName)
{
    // 添加双精度复数数组类型参数
    return m_cImplementation->AddParamArray(pcComplexData, iSize, pcCodeGenName, pcSizeName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(char *&sData, const char *pcCodeGenName)
{
    // 添加字符串类型参数
    return m_cImplementation->AddParam(sData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(QueryEnum &qData, const char *pcCodeGenName)
{
    // 添加查询枚举类型参数
    return m_cImplementation->AddParam(qData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(BooleanEnum &bData, const char *pcCodeGenName)
{
    // 添加布尔枚举类型参数
    return m_cImplementation->AddParam(bData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(SwitchEnum &sData, const char *pcCodeGenName)
{
    // 添加开关枚举类型参数
    return m_cImplementation->AddParam(sData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParamEnum(int *pidata, const char *pcCodeGenName, const char *pcEnumType)
{
    // 添加枚举类型参数
    return m_cImplementation->AddParamEnum(pidata, pcCodeGenName, pcEnumType);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParamFile(const char *pcName, const char *pcDescription, char *&sData, const char *pcValue)
{
    // 添加文件类型参数
    return m_cImplementation->AddParamFile(pcName, pcDescription, sData, pcValue);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(bool &bData, const char *pcCodeGenName)
{
    // 添加布尔类型参数
    return m_cImplementation->AddParam(bData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(Matrix<std::complex<float> > &cComplexMatrixData, const char *pcCodeGenName)
{
    // 添加单精度复数矩阵类型参数
    return m_cImplementation->AddParam(cComplexMatrixData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(Matrix<std::complex<double> > &cComplexMatrixData, const char *pcCodeGenName)
{
    // 添加双精度复数矩阵类型参数
    return m_cImplementation->AddParam(cComplexMatrixData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(Matrix<int> &cIntMatrixData, const char *pcCodeGenName)
{
    // 添加整数矩阵类型参数
    return m_cImplementation->AddParam(cIntMatrixData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(Matrix<float> &cFloatMatrixData, const char *pcCodeGenName)
{
    // 添加单精度浮点数矩阵类型参数
    return m_cImplementation->AddParam(cFloatMatrixData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(Matrix<double> &cDoubleMatrixData, const char *pcCodeGenName)
{
    // 添加双精度浮点数矩阵类型参数
    return m_cImplementation->AddParam(cDoubleMatrixData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(Matrix<bool> &cBoolMatrixData, const char *pcCodeGenName)
{
    // 添加布尔矩阵类型参数
    return m_cImplementation->AddParam(cBoolMatrixData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(FixedPointEnums::Sign &sData, const char *pcCodeGenName)
{
    // 添加定点数符号类型参数
    return m_cImplementation->AddParam(sData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(FixedPointEnums::QuantizationMode &sData, const char *pcCodeGenName)
{
    // 添加定点数量化模式参数
    return m_cImplementation->AddParam(sData, pcCodeGenName);
}

SystemVueModelBuilder::DFParam SystemVueModelBuilder::DFInterface::AddParam(FixedPointEnums::OverflowMode &sData, const char *pcCodeGenName)
{
    // 添加定点数溢出模式参数
    return m_cImplementation->AddParam(sData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(int &iData, const char *pcCodeGenName)
{
    // 添加int类型输入端口
    return m_cImplementation->AddInput(iData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(int &iData, const char *pcCodeGenName)
{
    // 添加int类型输出端口
    return m_cImplementation->AddOutput(iData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(double &dData, const char *pcCodeGenName)
{
    // 添加double类型输入端口
    return m_cImplementation->AddInput(dData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(double &dData, const char *pcCodeGenName)
{
    // 添加double类型输出端口
    return m_cImplementation->AddOutput(dData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(std::complex<double> &ComplexData, const char *pcCodeGenName)
{
    // 添加复数类型输入端口
    return m_cImplementation->AddInput(ComplexData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(std::complex<double> &ComplexData, const char *pcCodeGenName)
{
    // 添加复数类型输出端口
    return m_cImplementation->AddOutput(ComplexData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(int *&iData, const char *pcCodeGenName)
{
    // 添加int数组类型输入端口
    return m_cImplementation->AddInput(iData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(int *&iData, const char *pcCodeGenName)
{
    // 添加int数组类型输出端口
    return m_cImplementation->AddOutput(iData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(double *&dData, const char *pcCodeGenName)
{
    // 添加double数组类型输入端口
    return m_cImplementation->AddInput(dData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(double *&dData, const char *pcCodeGenName)
{
    // 添加double数组类型输出端口
    return m_cImplementation->AddOutput(dData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(std::complex<double> *&ComplexData, const char *pcCodeGenName)
{
    // 添加复数数组类型输入端口
    return m_cImplementation->AddInput(ComplexData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(std::complex<double> *&ComplexData, const char *pcCodeGenName)
{
    // 添加复数数组类型输出端口
    return m_cImplementation->AddOutput(ComplexData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBuffer<int> &circularBuffer, const char *pcCodeGenName)
{
    // 添加int类型CircularBuffer输入端口
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBuffer<int> &circularBuffer, const char *pcCodeGenName)
{
    // 添加int类型CircularBuffer输出端口
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBuffer<double> &circularBuffer, const char *pcCodeGenName)
{
    // 添加double类型CircularBuffer输入端口
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBuffer<double> &circularBuffer, const char *pcCodeGenName)
{
    // 添加double类型CircularBuffer输出端口
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBuffer<bool> &circularBuffer, const char *pcCodeGenName)
{
    // 添加bool类型CircularBuffer输入端口
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBuffer<bool> &circularBuffer, const char *pcCodeGenName)
{
    // 添加bool类型CircularBuffer输出端口
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBuffer<float> &circularBuffer, const char *pcCodeGenName)
{
    // 添加float类型CircularBuffer输入端口
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBuffer<float> &circularBuffer, const char *pcCodeGenName)
{
    // 添加float类型CircularBuffer输出端口
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBuffer<std::complex<float> > &circularBuffer, const char *pcCodeGenName)
{
    // 添加单精度复数类型CircularBuffer输入端口
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBuffer<std::complex<float> > &circularBuffer, const char *pcCodeGenName)
{
    // 添加单精度复数类型CircularBuffer输出端口
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

// 注释掉的CircularBufferBase类型端口方法
// SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBufferBase &circularBuffer, const char *pcCodeGenName)
// SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBufferBase &circularBuffer, const char *pcCodeGenName)

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBuffer<std::complex<double> > &circularBuffer, const char *pcCodeGenName)
{
    // 添加双精度复数类型CircularBuffer输入端口
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBuffer<std::complex<double> > &circularBuffer, const char *pcCodeGenName)
{
    // 添加双精度复数类型CircularBuffer输出端口
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(CircularBufferBus &circularBufferBus, const char *pcCodeGenName)
{
    // 添加CircularBufferBus类型输入端口
    return m_cImplementation->AddInput(circularBufferBus, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(CircularBufferBus &circularBufferBus, const char *pcCodeGenName)
{
    // 添加CircularBufferBus类型输出端口
    return m_cImplementation->AddOutput(circularBufferBus, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(EnvelopeCircularBuffer &envelopeData, const char *pcCodeGenName)
{
    // 添加EnvelopeCircularBuffer类型输入端口
    return m_cImplementation->AddInput(envelopeData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(EnvelopeCircularBuffer &envelopeData, const char *pcCodeGenName)
{
    // 添加EnvelopeCircularBuffer类型输出端口
    return m_cImplementation->AddOutput(envelopeData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::IntMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::IntMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::DoubleMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::DoubleMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::FloatMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::FloatMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::BoolMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::BoolMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::FComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::FComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::DComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::DComplexMatrixCircularBuffer &circularBuffer, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(circularBuffer, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddInput(SystemVueModelBuilder::EnvelopeMatrixCircularBuffer &envelopeData, const char *pcCodeGenName)
{
    return m_cImplementation->AddInput(envelopeData, pcCodeGenName);
}

SystemVueModelBuilder::DFPort SystemVueModelBuilder::DFInterface::AddOutput(SystemVueModelBuilder::EnvelopeMatrixCircularBuffer &envelopeData, const char *pcCodeGenName)
{
    return m_cImplementation->AddOutput(envelopeData, pcCodeGenName);
}

const char *SystemVueModelBuilder::DFInterface::GetLastError()
{
    // 获取最近的错误信息
    return m_cImplementation->GetLastError();
}

SystemVueModelBuilder::CDFInterfaceImplementation *SystemVueModelBuilder::DFInterface::GetImplementation() const
{
    // 获取底层实现对象指针
    return m_cImplementation;
}

bool SystemVueModelBuilder::DFInterface::SaveToConfig(ConfigManager &externalManager)
{
    // 将当前配置保存到外部配置管理器
    return m_cImplementation->SaveToConfig(externalManager);
}

bool SystemVueModelBuilder::DFInterface::LoadFromConfig()
{
    // 从配置文件加载配置
    return m_cImplementation->LoadFromConfig();
}

bool SystemVueModelBuilder::DFInterface::RemoveFromConfig()
{
    // 从配置中移除当前模型配置
    return m_cImplementation->RemoveFromConfig();
}

bool SystemVueModelBuilder::DFInterface::HasConfig()
{
    // 检查是否存在配置文件
    return m_cImplementation->HasConfig();
}
