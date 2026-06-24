#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QWebSocket>
#include <QtConcurrent>

#include "../Common/ISimRunner.h"
#include "../Common/LogExport.h"

// DDS通信协议结构体
struct DDSRequest {
    QString id;           // 请求ID
    QString action;       // 动作类型
    int errorCode;        // 错误码
    QJsonObject data;     // 数据体
    QString errorMsg;     // 错误信息
};

struct DDSResponse {
    QString id;           // 对应请求ID
    QString action;       // 动作类型
    int errorCode;        // 错误码 (0=成功)
    QJsonObject data;     // 返回数据
    QString errorMsg;     // 错误信息
};

class CommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit CommandHandler(QObject *parent = nullptr);
    ~CommandHandler();

    // 设置仿真器实例
    void setSimRunner(ISimRunner* simRunner);

    // 处理WebSocket消息
    DDSResponse processMessage(const QString& message);

    // 生成错误响应
    static DDSResponse createErrorResponse(const QString& id, const QString& action,
                                           int errorCode, const QString& errorMsg);

signals:
    // 异步响应信号，由 WebSocketServer 连接并发送
    void asyncResponse(const QString& message);

    // 引擎状态变化通知
    void engineStateChanged(int state);

    //异步发送日志
    void asyncLog(const QString& message);

private:
    // 命令处理方法
    //DDS服务 - 初始化: 加载模型
    DDSResponse handleLoadModel(const DDSRequest& request);
    //DDS服务 - 心跳: 获取状态
    DDSResponse handleGetModelStatus(const DDSRequest& request);
    //DDS服务 - 周期: 执行指令
    DDSResponse handleSimulationControl(const DDSRequest& request);
    void executeControlAsync(int ctrlType,const QString& reqId);
    //DDS服务 - 周期: 时间节拍
    DDSResponse handleCurrStepChanged(const DDSRequest& request);
    void executeStepAsync(int curStep, int errorHandle, const QString& reqId);

    //DDS服务 - 周期: 时间同步
    DDSResponse handleCurrTimeChanged(const DDSRequest& request);
    //DDS服务 - 周期: 可变参数配置
    DDSResponse handleVariableParameterChanged(const DDSRequest& request);
    //DDS服务 - 周期: 日志发送
    void onEngineLog(const char* level, const char* message, const char* timestamp);
    //DDS服务 - 周期: 输出数据完整路径上传
    void sendOutputPaths();

    //DDS服务 - 单次: 事件节拍
    DDSResponse handleEventStepChanged(const DDSRequest& request);
    void executeEventStepAsync(int eventID, int errorHandle, const QString &reqId);
    //DDS服务 - 周期: 事件ID接收
    DDSResponse handleEventDataRecv(const DDSRequest& request);
    void executeEventDataRecvAsync(QString eventID, int errorHandle, const QString &reqId, const QString& bits);

    // 解析请求
    DDSRequest parseRequest(const QString& message);
    // 序列化响应
    QString serializeResponse(const DDSResponse& response);

    // 状态转换
    QString getStateString(int state);

    ISimRunner* m_simRunner;

    // 模型信息
    QString m_modelVersion;
    QString m_modelName;
    bool m_modelLoaded;
};

#endif // COMMANDHANDLER_H
