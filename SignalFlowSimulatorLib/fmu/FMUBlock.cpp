// FMUBlock.cpp
#include "FMUBlock.h"
#include <QDebug>
#include <QFileInfo>

// 包含各种Buffer类型
#include "CircularBuffer.h"

namespace SystemVueModelBuilder {

FMUBlock::FMUBlock(const std::string &name)
    :Block(name)
{
    m_fmuManager = FMUManager::getInstance();
    SetBlockType(BlockType::PROCESSOR);
}

FMUBlock::~FMUBlock()
{
    qDebug() << "FMUBlock::~FMUBlock - 实例:" << m_instanceName;
}

void FMUBlock::setFMUConfig(const QString& guid, const QString& instanceName, int cmpId)
{
    m_guid = guid;
    m_instanceName = instanceName;
    m_cmpId = cmpId;
    SetName(instanceName.toStdString());  // 使用SetName方法设置名称
}

void FMUBlock::setDllPaths(const QVector<QString>& paths)
{
    m_dllPaths = paths;
}

void FMUBlock::addPortInfo(const PortMsg& port, int valueReference)
{
    PortInfo info;
    info.id = port.id;
    info.name = port.name;
    info.putType = port.putType;
    info.valueReference = valueReference;
    info.dataType = port.dataType;
    info.portRate = port.portRate;
    info.isOptional = port.isOptional;
    info.topProtId = port.topProtId;

    m_portInfos[port.id] = info;
    m_portNameToValueRef[port.name] = valueReference;

    if (port.putType == "in") {
        m_inputPortNames.append(port.name);
    } else if (port.putType == "out") {
        m_outputPortNames.append(port.name);
    }
}

void FMUBlock::addParameterInfo(const QString& paramName, int valueReference, const QString& value)
{
    ParamInfo info;
    info.name = paramName;
    info.valueReference = valueReference;
    info.value = value;
    m_paramInfos[paramName] = info;
}

int FMUBlock::getPortValueReference(const QString& portName) const
{
    return m_portNameToValueRef.value(portName, -1);
}

int FMUBlock::getParameterValueReference(const QString& paramName) const
{
    if (m_paramInfos.contains(paramName)) {
        return m_paramInfos[paramName].valueReference;
    }
    return -1;
}

bool FMUBlock::createPorts()
{
    qDebug() << "FMUBlock::createPorts - 实例:" << m_instanceName
             << "端口数:" << m_portInfos.size();

    int inputIdx = 0;
    int outputIdx = 0;

    for (auto it = m_portInfos.begin(); it != m_portInfos.end(); ++it) {
        const PortInfo& port = it.value();

        qDebug() << "创建端口:" << port.name
                 << "类型:" << port.putType
                 << "数据类型:" << (int)port.dataType
                 << "速率:" << port.portRate;

        if (port.putType == "in") {
            // 创建输入端口
            switch (port.dataType) {
            case PortMsg::PortDataType::REAL: {
                DoubleCircularBuffer* buffer = new DoubleCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::CIRCULAR_BUFFER_DOUBLE);
                qDebug() << "  添加输入端口(Double):" << port.name
                         << "索引:" << inputIdx;
                break;
            }
            case PortMsg::PortDataType::INT: {
                IntCircularBuffer* buffer = new IntCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::CIRCULAR_BUFFER_INT);
                qDebug() << "  添加输入端口(Double):" << port.name
                         << "索引:" << inputIdx;
                break;
            }
            case PortMsg::PortDataType::COMPLEX: {
                DComplexCircularBuffer* buffer = new DComplexCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::CIRCULAR_BUFFER_DCOMPLEX);
                qDebug() << "  添加输入端口(Complex):" << port.name;
                break;
            }
            case PortMsg::PortDataType::REAL_MATRIX: {
                DoubleMatrixCircularBuffer* buffer = new DoubleMatrixCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::MATRIX_DOUBLE);
                qDebug() << "  添加输入端口(Matrix Double):" << port.name;
                break;
            }
            case PortMsg::PortDataType::INT_MATRIX: {
                IntMatrixCircularBuffer* buffer = new IntMatrixCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::MATRIX_INT);
                qDebug() << "  添加输入端口(Matrix Double):" << port.name;
                break;
            }
            case PortMsg::PortDataType::COMPLEX_MATRIX: {
                DComplexMatrixCircularBuffer* buffer = new DComplexMatrixCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::MATRIX_DCOMPLEX);
                qDebug() << "  添加输入端口(Matrix Complex):" << port.name;
                break;
            }
            default:
                qDebug() << "  未知输入数据类型:" << (int)port.dataType;
                break;
            }
            inputIdx++;
        }
        else if (port.putType == "out") {
            // 创建输出端口
            switch (port.dataType) {
            case PortMsg::PortDataType::REAL:
            {
                DoubleCircularBuffer* buffer = new DoubleCircularBuffer();
                AddOutputPort(port.name.toStdString(), *buffer, 1,
                              DataType::CIRCULAR_BUFFER_DOUBLE);
                qDebug() << "  添加输出端口(Double):" << port.name
                         << "索引:" << outputIdx;
                break;
            }
            case PortMsg::PortDataType::INT:{
                IntCircularBuffer* buffer = new IntCircularBuffer();
                AddInputPort(port.name.toStdString(), *buffer, 1,
                             DataType::CIRCULAR_BUFFER_INT);
                qDebug() << "  添加输入端口(Double):" << port.name
                         << "索引:" << inputIdx;
                break;
            }
            case PortMsg::PortDataType::COMPLEX: {
                DComplexCircularBuffer* buffer = new DComplexCircularBuffer();
                AddOutputPort(port.name.toStdString(), *buffer, 1,
                              DataType::CIRCULAR_BUFFER_DCOMPLEX);
                qDebug() << "  添加输出端口(Complex):" << port.name;
                break;
            }
            case PortMsg::PortDataType::REAL_MATRIX:
            {
                DoubleMatrixCircularBuffer* buffer = new DoubleMatrixCircularBuffer();
                AddOutputPort(port.name.toStdString(), *buffer, 1,
                              DataType::MATRIX_DOUBLE);
                qDebug() << "  添加输出端口(Matrix Double):" << port.name;
                break;
            }
            case PortMsg::PortDataType::INT_MATRIX:{
                IntMatrixCircularBuffer* buffer = new IntMatrixCircularBuffer();
                AddOutputPort(port.name.toStdString(), *buffer, 1,
                             DataType::MATRIX_INT);
                qDebug() << "  添加输入端口(Matrix Double):" << port.name;
                break;
            }
            case PortMsg::PortDataType::COMPLEX_MATRIX: {
                DComplexMatrixCircularBuffer* buffer = new DComplexMatrixCircularBuffer();
                AddOutputPort(port.name.toStdString(), *buffer, 1,
                              DataType::MATRIX_DCOMPLEX);
                qDebug() << "  添加输出端口(Matrix Complex):" << port.name;
                break;
            }
            default:
                qDebug() << "  未知输出数据类型:" << (int)port.dataType;
                break;
            }
            outputIdx++;
        }
    }
    //输入端口数为0，为信号源
    if(inputIdx == 0 && outputIdx != 0) {
        SetBlockType(Block::BlockType::SOURCE);
    }
    else if(inputIdx != 0 && outputIdx != 0) {
        SetBlockType(Block::BlockType::PROCESSOR);
    }
    else if(inputIdx != 0 && outputIdx == 0){
        SetBlockType(Block::BlockType::SINK);
    }

    qDebug() << "FMUBlock端口创建完成 - 输入端口数:" << inputIdx
             << "输出端口数:" << outputIdx;
    return true;
}

bool FMUBlock::Initialize()
{
    qDebug() << "FMUBlock::Initialize - 实例:" << m_instanceName << "GUID:" << m_guid;

    if (m_guid.isEmpty()) {
        LOG_ERROR("FMU GUID为空:", m_instanceName.toStdString());
        return false;
    }

    // 创建输入输出端口
    if (!createPorts()) {
        LOG_ERROR("FMU创建端口失败:", m_instanceName.toStdString());
        return false;
    }

    // 验证FMU实例是否存在（由FMUManager管理）
    if (!m_fmuManager->hasInstance(m_guid)) {
        LOG_ERROR("FMU实例不存在，GUID:", m_guid.toStdString());
        return false;
    }

    m_isInitialized = true;
    return true;
}

bool FMUBlock::setupStaticParameters()
{
    qDebug() << "FMUBlock::setupStaticParameters - 实例:" << m_instanceName;

    if (m_paramInfos.isEmpty()) {
        qDebug() << "没有静态参数需要设置";
        return true;
    }

    // 分类存储不同类型的参数
    std::vector<QString> realNames;
    std::vector<double> realValues;
    std::vector<QString> intNames;
    std::vector<int> intValues;
    std::vector<QString> boolNames;
    std::vector<bool> boolValues;
    std::vector<QString> stringNames;
    std::vector<QString> stringValues;

    for (auto it = m_paramInfos.begin(); it != m_paramInfos.end(); ++it) {
        const ParamInfo& param = it.value();

        qDebug() << "处理FMU参数:" << param.name
                 << "valueReference:" << param.valueReference
                 << "值:" << param.value;

        // 尝试解析参数类型
        bool ok;
        double doubleVal = param.value.toDouble(&ok);
        if (ok) {
            realNames.push_back(param.name);
            realValues.push_back(doubleVal);
            continue;
        }

        if (param.value == "true" || param.value == "false") {
            boolNames.push_back(param.name);
            boolValues.push_back(param.value == "true");
            continue;
        }

        int intVal = param.value.toInt(&ok);
        if (ok) {
            intNames.push_back(param.name);
            intValues.push_back(intVal);
            continue;
        }

        // 默认为字符串
        stringNames.push_back(param.name);
        stringValues.push_back(param.value);
    }

    // 批量设置参数
    bool success = true;

    if (!realNames.empty()) {
        if (!m_fmuManager->setReals(m_guid, realNames, realValues)) {
            LOG_ERROR("FMU批量设置Real参数失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置Real参数成功，数量:" << realNames.size();
        }
    }

    if (!intNames.empty()) {
        if (!m_fmuManager->setIntegers(m_guid, intNames, intValues)) {
            LOG_ERROR("FMU批量设置Integer参数失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置Integer参数成功，数量:" << intNames.size();
        }
    }

    if (!boolNames.empty()) {
        if (!m_fmuManager->setBooleans(m_guid, boolNames, boolValues)) {
            LOG_ERROR("FMU批量设置Boolean参数失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置Boolean参数成功，数量:" << boolNames.size();
        }
    }

    if (!stringNames.empty()) {
        if (!m_fmuManager->setStrings(m_guid, stringNames, stringValues)) {
            LOG_ERROR("FMU批量设置String参数失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置String参数成功，数量:" << stringNames.size();
        }
    }

    return success;
}

bool FMUBlock::Setup()
{
    qDebug() << "FMUBlock::Setup - 实例:" << m_instanceName;

    if (!m_isInitialized) {
        LOG_ERROR("FMUBlock未初始化:", m_instanceName.toStdString());
        return false;
    }

    // 设置静态参数
    if (!setupStaticParameters()) {
        LOG_ERROR("FMU设置静态参数失败:", m_instanceName.toStdString());
        return false;
    }
    // 重置当前步数和时间
    m_currentStep = 0;
    m_currentTime = m_startTime;
    m_startTime = getSimu().startTime;
    m_stopTime = getSimu().stopTime;
    m_timeInterval = getSimu().time_Interval;
    m_numSamples = getSimu().num_Samples;
    m_samplingRate = getSimu().samplingRate;
    qDebug() << "FMUBlock::Setup - startTime: " << getSimu().startTime;
    qDebug() << "FMUBlock::Setup - stopTime: " << getSimu().stopTime;
    qDebug() << "FMUBlock::Setup - time_Interval: " << getSimu().time_Interval;
    qDebug() << "FMUBlock::Setup - num_Samples: " << getSimu().num_Samples;
    qDebug() << "FMUBlock::Setup - samplingRate: " << getSimu().samplingRate;

    Block::Setup();

    m_isSetup = true;
    return true;
}

bool FMUBlock::readInputsAndSetToFMU()
{
    if (m_inputPortNames.isEmpty()) {
        return true;
    }

    // 分类存储不同类型的输入数据
    std::vector<QString> realNames;
    std::vector<double> realValues;
    std::vector<QString> intNames;
    std::vector<int> intValues;
    std::vector<QString> boolNames;
    std::vector<bool> boolValues;

    // 遍历所有输入端口，从Buffer读取数据
    for (const QString& portName : m_inputPortNames) {
        int valueRef = getPortValueReference(portName);
        if (valueRef == -1) {
            qDebug() << "FMU输入端口无valueReference:" << portName;
            continue;
        }

        // 获取输入端口信息
        PortInfo* portInfo = nullptr;
        for (auto& info : m_portInfos) {
            if (info.name == portName) {
                portInfo = &info;
                break;
            }
        }
        if (!portInfo) continue;

        // 从输入Buffer读取数据
        BufferReader* reader = GetInputPort(portName.toStdString());
        if (!reader) {
            qDebug() << "FMU输入端口未连接:" << portName;
            continue;
        }

        // 检查是否有数据可用
        if (!reader->HasDataAvailable()) {
            qDebug() << "FMU输入端口无数据:" << portName;
            continue;
        }

        qDebug() << "FMUBlock::readInputsAndSetToFMU - portInfo: " << portInfo->name;
        qDebug() << "FMUBlock::readInputsAndSetToFMU - portInfo is real? : "
                 << (portInfo->dataType == PortMsg::PortDataType::REAL ? "true" : "false");
        // 根据数据类型读取
        switch (portInfo->dataType) {
        case PortMsg::PortDataType::REAL:
        {
            std::vector<double> data;
            qDebug() << "FMUBlock::readInputsAndSetToFMU - data: " << (data.empty() ? "true" : "false");
            if (reader->ReadData(data)) {
                realNames.push_back(portName);
                realValues.push_back(data[0]);
                qDebug() << "读取输入(Real):" << portName << "值:" << data[0];
            }
            break;
        }
        case PortMsg::PortDataType::INT: {
            std::vector<int> data;
            if (reader->ReadData(data) && !data.empty()) {
                intNames.push_back(portName);
                intValues.push_back(data[0]);
                qDebug() << "读取输入(int):" << portName << "值:" << data[0];
            }
            break;
        }
//        case PortMsg::PortDataType::COMPLEX: {
//            // 复数类型：读取两个double值（实部和虚部）
//            std::vector<double> data;
//            if (reader->ReadData(data) && data.size() >= 2) {
//                // 如果FMU需要复数，可能需要特殊处理
//                // 暂时作为两个Real值处理
//                realNames.push_back(portName + "_real");
//                realValues.push_back(data[0]);
//                realNames.push_back(portName + "_imag");
//                realValues.push_back(data[1]);
//                qDebug() << "读取输入(Complex):" << portName
//                         << "实部:" << data[0] << "虚部:" << data[1];
//            }
//            break;
//        }
//        case PortMsg::PortDataType::BOOLEAN: {
//            std::vector<bool> data;
//            if (reader->ReadData(data) && !data.empty()) {
//                boolNames.push_back(portName);
//                boolValues.push_back(data[0]);
//                qDebug() << "读取输入(Boolean):" << portName << "值:" << data[0];
//            }
//            break;
//        }
        case PortMsg::PortDataType::REAL_MATRIX:{
            // 矩阵类型：读取矩阵数据
            std::vector<std::vector<double>> matrixData;
            // 注意：这里需要根据实际的矩阵读取接口调整
            qDebug() << "读取输入(Matrix):" << portName << "暂未实现矩阵读取";
            break;
        }
        case PortMsg::PortDataType::INT_MATRIX: {
            // 矩阵类型：读取矩阵数据
            std::vector<std::vector<int>> matrixData;
            // 注意：这里需要根据实际的矩阵读取接口调整
            qDebug() << "读取输入(Matrix):" << portName << "暂未实现矩阵读取";
            break;
        }
        case PortMsg::PortDataType::COMPLEX_MATRIX: {
            qDebug() << "读取输入(Complex Matrix):" << portName << "暂未实现";
            break;
        }
        default:
            qDebug() << "FMU输入不支持的数据类型:" << (int)portInfo->dataType;
            break;
        }
    }

    // 批量设置输入数据到FMU
    bool success = true;

    if (!realNames.empty()) {
        if (!m_fmuManager->setReals(m_guid, realNames, realValues)) {
            LOG_ERROR("FMU批量设置Real输入失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置Real输入成功，数量:" << realNames.size() << ",值: " << realValues[0];
        }
    }

    if (!intNames.empty()) {
        if (!m_fmuManager->setIntegers(m_guid, intNames, intValues)) {
            LOG_ERROR("FMU批量设置Integer输入失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置Integer输入成功，数量:" << intNames.size();
        }
    }

    if (!boolNames.empty()) {
        if (!m_fmuManager->setBooleans(m_guid, boolNames, boolValues)) {
            LOG_ERROR("FMU批量设置Boolean输入失败:", m_instanceName.toStdString());
            success = false;
        } else {
            qDebug() << "批量设置Boolean输入成功，数量:" << boolNames.size();
        }
    }

    return success;
}

bool FMUBlock::readOutputsFromFMUAndWrite()
{
    if (m_outputPortNames.isEmpty()) {
        return true;
    }

    // 分类存储不同类型的输出数据
    std::vector<QString> realNames;
    std::vector<QString> intNames;
    std::vector<QString> boolNames;
    std::vector<QString> stringNames;

    // 先收集需要获取的输出端口名称
    for (const QString& portName : m_outputPortNames) {
        PortInfo* portInfo = nullptr;
        for (auto& info : m_portInfos) {
            if (info.name == portName) {
                portInfo = &info;
                break;
            }
        }
        if (!portInfo) continue;

        switch (portInfo->dataType) {
        case PortMsg::PortDataType::REAL:
            realNames.push_back(portName);
            break;
        case PortMsg::PortDataType::INT:
            intNames.push_back(portName);
            break;
//        case PortMsg::PortDataType::BOOLEAN:
//            boolNames.push_back(portName);
//            break;
//        case PortMsg::PortDataType::COMPLEX:
//            // 复数可能需要两个Real值
//            realNames.push_back(portName + "_real");
//            realNames.push_back(portName + "_imag");
//            break;
        default:
            break;
        }
    }

    // 批量从FMU获取输出值
    std::vector<double> realValues;
    std::vector<int> intValues;
    std::vector<bool> boolValues;
    std::vector<QString> stringValues;

    if (!realNames.empty()) {
        realValues = m_fmuManager->getReals(m_guid, realNames);
        qDebug() << "批量获取Real输出成功，数量:" << realValues.size() << ",值: " << realValues[realValues.size() - 1];
    }

    if (!intNames.empty()) {
        intValues = m_fmuManager->getIntegers(m_guid, intNames);
        qDebug() << "批量获取int输出成功，数量:" << intValues.size();
    }

    if (!boolNames.empty()) {
        boolValues = m_fmuManager->getBooleans(m_guid, boolNames);
        qDebug() << "批量获取bool输出成功，数量:" << boolValues.size();
    }

    if (!stringNames.empty()) {
        stringValues = m_fmuManager->getStrings(m_guid, stringNames);
        qDebug() << "批量获取String输出成功，数量:" << stringValues.size();
    }

    // 将获取到的值写入输出Buffer
    size_t realIndex = 0;
    size_t intIndex = 0;
//    size_t boolIndex = 0;
//    size_t stringIndex = 0;

    for (const QString& portName : m_outputPortNames) {
        // 获取输出端口信息
        PortInfo* portInfo = nullptr;
        for (auto& info : m_portInfos) {
            if (info.name == portName) {
                portInfo = &info;
                break;
            }
        }
        if (!portInfo) continue;

        // 获取输出Buffer
        Buffer* buffer = GetOutputPort(portName.toStdString());
        if (!buffer) {
            qDebug() << "FMU输出端口未连接:" << portName;
            continue;
        }

        // 检查是否有足够空间
        if (buffer->GetBufferFreeSpace() == 0) {
            qDebug() << "FMU输出Buffer已满:" << portName;
            continue;
        }

        // 根据数据类型写入Buffer
        switch (portInfo->dataType) {
        case PortMsg::PortDataType::REAL:{
            if (realIndex < realValues.size()) {
                std::vector<double> data = {realValues[realIndex++]};
                buffer->WriteData(data);
                qDebug() << "buffer: ";
                qDebug() << "buffer TotalWritten: " << buffer->GetTotalWritten();
                qDebug() << "buffer BufferFreeSpace: " << buffer->GetBufferFreeSpace();
                qDebug() << "buffer UsedSpace: " << buffer->GetUsedSpace();
                qDebug() << "buffer ReaderCount: " << buffer->GetReaderCount();
                qDebug() << "写入输出(Real):" << portName << "值:" << data[0];
            }
            break;
        }
        case PortMsg::PortDataType::INT: {
            if (intIndex < realValues.size()) {
                std::vector<int> data = {intValues[intIndex++]};
                buffer->WriteData(data);
                qDebug() << "写入输出(Int):" << portName << "值:" << data[0];
            }
            break;
        }
//        case PortMsg::PortDataType::BOOLEAN: {
//            if (boolIndex < boolValues.size()) {
//                std::vector<bool> data = {boolValues[boolIndex++]};
//                buffer->Write(data);
//                qDebug() << "写入输出(Boolean):" << portName << "值:" << data[0];
//            }
//            break;
//        }
//        case PortMsg::PortDataType::COMPLEX: {
//            // 复数：写入实部和虚部
//            if (realIndex + 1 < realValues.size()) {
//                std::vector<double> data = {realValues[realIndex++], realValues[realIndex++]};
//                buffer->Write(data);
//                qDebug() << "写入输出(Complex):" << portName
//                         << "实部:" << data[0] << "虚部:" << data[1];
//            }
//            break;
//        }
        case PortMsg::PortDataType::REAL_MATRIX:
        case PortMsg::PortDataType::INT_MATRIX: {
            // 矩阵类型需要特殊处理
            qDebug() << "写入输出(Matrix):" << portName << "暂未实现矩阵写入";
            break;
        }
        case PortMsg::PortDataType::COMPLEX_MATRIX: {
            qDebug() << "写入输出(Complex Matrix):" << portName << "暂未实现";
            break;
        }
        default:
            qDebug() << "FMU输出不支持的数据类型:" << (int)portInfo->dataType;
            break;
        }
    }

    return true;
}

bool FMUBlock::Run()
{
    qDebug() << "FMUBlock::Run - 实例:" << m_instanceName
             << "当前步数:" << m_currentStep << "/" << m_numSamples
             << "当前时间:" << m_currentTime
             << "guid:" << m_guid
             << "fmuManager:" << m_fmuManager;

    if (!m_isSetup) {
        LOG_ERROR("FMUBlock未Setup:", m_instanceName.toStdString());
        return false;
    }

    // 1. 从输入Buffer读取数据并设置到FMU
    readInputsAndSetToFMU();

    // 2. 执行FMU的一步计算
    double stepSize = m_timeInterval;

    if (!m_fmuManager->dostep(m_guid, m_currentTime, stepSize)) {
        LOG_ERROR("FMU doStep失败:", m_instanceName.toStdString());
        return false;
    }

    // 3. 从FMU读取输出并写入输出Buffer
    readOutputsFromFMUAndWrite();
    m_currentStep++;
    m_currentTime += stepSize;

    return true;
}

//bool FMUBlock::Done()
//{
//    qDebug() << "FMUBlock::Done - 实例:" << m_instanceName;
//    return true;
//}

//bool FMUBlock::Stop()
//{
//    qDebug() << "FMUBlock::Stop - 实例:" << m_instanceName;
//    //禁用所有输出端口的自动触发
//    for(const auto& outputPort : GetOutputPorts()) {
//        Buffer* buffer = outputPort.second;
//        if(buffer) {
//            buffer->SetUpstreamDone(true);
//            buffer->SetDownstreamDone(true);
//        }
//        if(buffer->IsBusType(buffer->GetDataType())) {
//            for(auto& bus : buffer->GetBusConnections()) {
//                bus.bridgeWriter->SetUpstreamDone(true);
//                bus.bridgeWriter->SetDownstreamDone(true);
//            }
//        }
//    }


//;
////    qDebug() << "Block '" << QString::fromStdString(m_name) << "': Stopped - exited data flow loop";
//    return true;
//    return true;
//}

} // namespace SystemVueModelBuilder
