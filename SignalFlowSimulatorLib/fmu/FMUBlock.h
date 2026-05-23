// FMUBlock.h
#ifndef FMUBLOCK_H
#define FMUBLOCK_H

#include <Block.h>
#include "FMUManager.h"
#include <QMap>
#include <QString>
#include <vector>

namespace SystemVueModelBuilder {

class FMUBlock : public Block
{
public:
    FMUBlock(const std::string& name);
    ~FMUBlock();

    // 初始化FMU块（创建输入输出端口）
    bool Initialize() override;

    // 设置参数（Setup阶段调用）
    bool Setup() override;

    // 执行一步仿真
    bool Run() override;

//    // 清理
//    bool Done() override;

//    // 停止
//    bool Stop() override;

    // 设置FMU配置信息
    void setFMUConfig(const QString& guid, const QString& instanceName, int cmpId);

    // 设置库路径列表
    void setDllPaths(const QVector<QString>& paths);

    // 添加端口信息
    void addPortInfo(const PortMsg& port, int valueReference);

    // 添加参数信息
    void addParameterInfo(const QString& paramName, int valueReference, const QString& value);

    // 获取端口valueReference
    int getPortValueReference(const QString& portName) const;

    // 获取参数valueReference
    int getParameterValueReference(const QString& paramName) const;

private:
    // 创建输入输出端口（根据端口类型创建对应的Buffer）
    bool createPorts();

    // 从输入Buffer读取数据并设置到FMU
    bool readInputsAndSetToFMU();

    // 从FMU读取输出并写入输出Buffer
    bool readOutputsFromFMUAndWrite();

    // 设置静态参数到FMU
    bool setupStaticParameters();

private:
    QString m_guid;                          // FMU的GUID
    QString m_instanceName;                  // 实例名称
    int m_cmpId = -1;                        // 组件ID
    QVector<QString> m_dllPaths;             // 动态库路径列表

    // 端口信息存储
    struct PortInfo {
        int id = -1;
        QString name;
        QString putType;                      // in/out
        int valueReference = -1;
        PortMsg::PortDataType dataType;
        int portRate = 0;
        bool isOptional = false;
        int topProtId = -1;
    };
    QMap<int, PortInfo> m_portInfos;          // 端口ID -> 端口信息
    QMap<QString, int> m_portNameToValueRef;  // 端口名 -> valueReference

    // 参数信息存储
    struct ParamInfo {
        QString name;
        int valueReference = -1;
        QString value;
        QString dataType;
    };
    QMap<QString, ParamInfo> m_paramInfos;    // 参数名 -> 参数信息

    // 输入/输出端口名称列表
    QStringList m_inputPortNames;
    QStringList m_outputPortNames;

    // FMU管理器引用（不修改FMUManager，直接使用单例）
    FMUManager* m_fmuManager = nullptr;

    bool m_isInitialized = false;
    bool m_isSetup = false;

    // 仿真参数
    double m_startTime = 0.0;
    double m_stopTime = 0.0;
    double m_samplingRate = 1.0;
    double m_timeInterval = 1.0;
    int m_numSamples = 0;

    // 当前执行状态
    int m_currentStep = 0;      // 当前步数
    double m_currentTime = 0.0; // 当前仿真时间
};

} // namespace SystemVueModelBuilder

#endif // FMUBLOCK_H
