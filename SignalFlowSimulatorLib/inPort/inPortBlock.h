#ifndef INPORTBLOCK_H
#define INPORTBLOCK_H

#include <Block.h>
#include <QMap>
#include <QString>
#include <vector>
#include <queue>

namespace SystemVueModelBuilder {

class inPortBlock : public Block
{
public:
    inPortBlock(const std::string& name);
    inPortBlock() = default;
    ~inPortBlock();

    // 初始化inPort块（创建输入输出端口）
    bool Initialize() override;

    // 设置参数（Setup阶段调用）
    bool Setup() override;

    // 执行一步仿真
    bool Run() override;

    // 清理
    bool Done() override;

    // 停止
    bool Stop() override;

    // 添加端口信息
    void addPortInfo(const PortMsg& port);

    // 添加参数信息
    void addParameterInfo(const QString& paramName, const QString& value);


    // 创建输入输出端口（根据端口类型创建对应的Buffer）
    bool createPorts();

    bool readOutputsAndWrite();


    QString m_instanceName;                  // 实例名称
    int m_cmpId = -1;                        // 组件ID

    // 端口信息存储
    struct PortInfo {
        int id = -1;
        QString name;
        QString putType;                      // in/out
        PortMsg::PortDataType dataType;
        int portRate = 0;
        bool isOptional = false;
        int topProtId = -1;
    };
    QMap<int, PortInfo> m_portInfos;          // 端口ID -> 端口信息

    // 参数信息存储
    struct ParamInfo {
        QString name;
        QString value;
        QString dataType;
    };
    QMap<QString, ParamInfo> m_paramInfos;    // 参数名 -> 参数信息

    // 输入/输出端口名称列表
    QStringList m_inputPortNames;
    QStringList m_outputPortNames;

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

    std::queue<int> m_intQueue;
    std::queue<double> m_doubleQueue;
    std::queue<std::complex<double>> m_dcomplexQueue;


    Block* m_block = nullptr;
};

} // namespace SystemVueModelBuilder

#endif // INPORTBLOCK_H
