#include "CommandHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

CommandHandler::CommandHandler(QObject *parent)
    : QObject(parent)
    , m_simRunner(nullptr)
    , m_modelVersion("v1.0")
    , m_modelLoaded(false)
{
}

CommandHandler::~CommandHandler()
{
}

void CommandHandler::setSimRunner(ISimRunner* simRunner)
{
    m_simRunner = simRunner;

    m_simRunner->SetLogCallback([this](const char* level, const char* message, const char* timestamp) {
        onEngineLog(level, message, timestamp);
    });

    qDebug() << "setSimRunner: SetLogCallback 完成";
}

DDSRequest CommandHandler::parseRequest(const QString& message)
{
    DDSRequest request;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        request.errorCode = -1;
        request.errorMsg = "JSON解析失败: " + error.errorString();
        return request;
    }

    QJsonObject obj = doc.object();
    request.id = obj["id"].toString();
    request.action = obj["action"].toString();
    request.errorCode = obj["errorCode"].toInt(0);
    request.data = obj["data"].toObject();
    request.errorMsg = obj["errorMsg"].toString();

    return request;
}

QString CommandHandler::serializeResponse(const DDSResponse& response)
{
    QJsonObject obj;
    obj["id"] = response.id;
    obj["action"] = response.action;
    obj["errorCode"] = response.errorCode;
    obj["data"] = response.data;
    obj["errorMsg"] = response.errorMsg;

    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

DDSResponse CommandHandler::processMessage(const QString& message)
{
    DDSRequest request = parseRequest(message);

    // 解析失败
    if (request.errorCode == -1) {
        return createErrorResponse(request.id, request.action, -1, request.errorMsg);
    }

    // 路由命令
    if (request.action == "loadModel") {
        return handleLoadModel(request);
    } else if (request.action == "getModelStatus") {
        return handleGetModelStatus(request);
    } else if (request.action == "simulationControlChanged") {
        return handleSimulationControl(request);
    } else if (request.action == "currStepChanged") {
        return handleCurrStepChanged(request);
    } else if (request.action == "eventStepChanged") {
        return handleEventStepChanged(request);
    } else if (request.action == "eventDataRecv") {
        return handleEventDataRecv(request);
    }

    else {
        return createErrorResponse(request.id, request.action, -2,
                                   "未知的action: " + request.action);
    }
}

DDSResponse CommandHandler::handleLoadModel(const DDSRequest& request)
{
    DDSResponse response;
    response.id = request.id;
    response.action = request.action;

    if (!m_simRunner) {
        response.errorCode = 1;
        response.errorMsg = "仿真器实例未创建";
        return response;
    }

    // 从 data 中提取 linkFiles
    QJsonArray linkFilesArray = request.data["linkFiles"].toArray();

    qDebug() << "handleLoadModel - linkFilesArray: " << linkFilesArray;

    if (linkFilesArray.isEmpty()) {
        response.errorCode = 1;
        response.errorMsg = "缺少linkFiles参数";
        return response;
    }

    // 转换为 const char** 格式
    std::vector<QByteArray> fileBytes;
    std::vector<const char*> filePtrs;

    for (const QJsonValue& file : linkFilesArray) {
        fileBytes.push_back(file.toString().toUtf8());
        filePtrs.push_back(fileBytes.back().constData());
    }
    qDebug() << "handleLoadModel - filePtrs: " << *fileBytes.data();
    qDebug() << "handleLoadModel - filePtrs size: " << filePtrs.size();

//    // 设置链路文件
    m_simRunner->SetLinkFiles(filePtrs.data(), static_cast<int>(filePtrs.size()));

//    // 初始化模型
    bool success = m_simRunner->start();
    success = m_simRunner->Initialize();
    success = m_simRunner->Setup();

    if (success) {
        m_modelLoaded = true;
        response.errorCode = 0;
        response.errorMsg = "";

//        LOG_INFO("模型加载成功，链路文件数: ", filePtrs.size());
    } else {
        response.errorCode = 1;
        response.errorMsg = "模型加载失败";

        LOG_ERROR("模型加载失败");
    }

    return response;
}

DDSResponse CommandHandler::handleGetModelStatus(const DDSRequest& request)
{
    DDSResponse response;
    response.id = request.id;
    response.action = request.action;

    if (!m_simRunner || !m_modelLoaded) {
        response.errorCode = 1;
        response.errorMsg = "模型未加载";
        return response;
    }

    // 调用引擎接口获取状态
//    int engineState = 1;
    int engineState = m_simRunner->GetModelStatus();

    if(engineState < 0) {
        response.errorCode = 1;
        response.data["engineState"] = engineState;
    }

    response.errorCode = 0;
    response.data["status"] = engineState;

    LOG_INFO("查询模型状态: ", engineState);

    return response;
}

DDSResponse CommandHandler::handleSimulationControl(const DDSRequest& request)
{
    DDSResponse ackResponse;
    ackResponse.id = request.id;
    ackResponse.action = request.action;

    if (!m_simRunner || !m_modelLoaded) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "模型未加载";
        return ackResponse;
    }

    int ctrlType = request.data["ctrlType"].toInt(-1);

    if (ctrlType == -1) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "缺少ctrlType参数";
        return ackResponse;
    }

    // 先回复 ACK（指令已接收）
    ackResponse.errorCode = 0;
//    ackResponse.errorMsg = "指令已接收";
    ackResponse.data["ctrlType"] = ctrlType;

    // 异步执行控制指令
    QString reqId = request.id;

    QtConcurrent::run([this, ctrlType,reqId]() {
        executeControlAsync(ctrlType,reqId);
    });

    return ackResponse;  // 立即返回 ACK
}

void CommandHandler::executeControlAsync(int ctrlType, const QString& reqId)
{
    LOG_INFO("executeControlAsync started:",ctrlType);
    bool success = false;
    QString ctrlDesc;

    switch (ctrlType) {
    case 0:
        success = true;
        ctrlDesc = "无效控制";
        break;
    case 1:
//        success = m_simRunner->Initialize();
        success = true;
        ctrlDesc = "初始化";
        break;
    case 2:
//        success = m_simRunner->Setup();
        success = true;
        ctrlDesc = "开始";
        break;
    case 3:
        //时间节拍
        success = m_simRunner->run();
        success = true;
        ctrlDesc = "开始节拍调度";
        break;
    case 4:
        //事件节拍
//        success = m_simRunner->OnEventStepChanged();
        success = true;
        ctrlDesc = "事件调度";
    case 6:
        // success = m_simRunner->pause();
        ctrlDesc = "暂停";
        break;
    case 7:
        success = m_simRunner->Stop();
        ctrlDesc = "停止";
        break;
    default:
        break;
    }

    // 构建执行结果响应
    QJsonObject respObj;
    respObj["id"] = reqId;
    respObj["action"] = "simulationControlChanged";
    respObj["errorCode"] = success ? 0 : 1;
    respObj["errorMsg"] = success ? "" : QString("%1失败").arg(ctrlDesc);

    QJsonObject data;
    data["ctrlType"] = ctrlType;
    respObj["data"] = data;

    QJsonDocument doc(respObj);
    QString responseStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // 发送异步执行结果
    QMetaObject::invokeMethod(this, [this, responseStr]() {
        emit asyncResponse(responseStr);
    }, Qt::QueuedConnection);

    // 通知状态变化
    if (success) {
        int newState = m_simRunner->GetModelStatus();
        emit engineStateChanged(newState);
    }

    LOG_INFO("控制指令异步执行完成: ", ctrlDesc.toStdString(),
             " result=", success ? "成功" : "失败");
}

DDSResponse CommandHandler::handleCurrStepChanged(const DDSRequest& request)
{
    DDSResponse ackResponse;
    ackResponse.id = request.id;
    ackResponse.action = request.action;

    if (!m_simRunner || !m_modelLoaded) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "模型未加载";
        return ackResponse;
    }

    // 提取节拍参数
    int curStep = request.data["curStep"].toInt(-1);
    int errorHandle = request.data["errorHandle"].toInt(0);

    if (curStep == -1) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "缺少curStep参数";
        return ackResponse;
    }

    // 先回复 ACK（指令已接收）
    ackResponse.errorCode = 0;
//    ackResponse.errorMsg = "节拍指令已接收";
    ackResponse.errorMsg = "";
    ackResponse.data["curStep"] = curStep;
    ackResponse.data["errorHandle"] = errorHandle;

    // 异步执行节拍控制
    QString reqId = request.id;

    QtConcurrent::run([this, curStep, errorHandle, reqId]() {
        executeStepAsync(curStep, errorHandle, reqId);
    });

    return ackResponse;  // 立即返回 ACK
}

void CommandHandler::executeStepAsync(int curStep, int errorHandle, const QString &reqId)
{
    bool success = m_simRunner->OnCurrStepChanged(curStep);
//    bool success = m_simRunner->run();

    // 构建执行结果响应
    QJsonObject respObj;
    respObj["id"] = reqId;
    respObj["action"] = "currStepChanged";
    respObj["errorCode"] = success ? 0 : 1;
    respObj["errorMsg"] = success ? "" : "节拍发送失败";

    QJsonObject data;
    data["curStep"] = curStep;
    data["errorHandle"] = errorHandle;
    data["completed"] = success;
    respObj["data"] = data;

    QJsonDocument doc(respObj);
    QString responseStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // 发送异步执行结果
    QMetaObject::invokeMethod(this, [this, responseStr]() {
        emit asyncResponse(responseStr);
    }, Qt::QueuedConnection);

    // 发送结果数据完整路径
    sendOutputPaths();

    LOG_INFO("节拍异步执行完成: curStep=", curStep,
             " errorHandle=", errorHandle,
             " result=", success ? "成功" : "失败");
}

DDSResponse CommandHandler::handleEventStepChanged(const DDSRequest &request)
{
    DDSResponse ackResponse;
    ackResponse.id = request.id;
    ackResponse.action = request.action;

    if (!m_simRunner || !m_modelLoaded) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "模型未加载";
        return ackResponse;
    }

    // 提取节拍参数
    int eventID = request.data["eventID"].toInt(-1);
    int errorHandle = request.data["errorHandle"].toInt(0);

    if (eventID == -1) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "缺少eventID参数";
        return ackResponse;
    }

    // 先回复 ACK（指令已接收）
    ackResponse.errorCode = 0;
//    ackResponse.errorMsg = "节拍指令已接收";
    ackResponse.errorMsg = "";
    ackResponse.data["eventID"] = eventID;
    ackResponse.data["errorHandle"] = errorHandle;

    // 异步执行节拍控制
    QString reqId = request.id;

    QtConcurrent::run([this, eventID, errorHandle, reqId]() {
        executeEventStepAsync(eventID, errorHandle, reqId);
    });

    return ackResponse;  // 立即返回 ACK
}

void CommandHandler::executeEventStepAsync(int eventID, int errorHandle, const QString &reqId)
{
//    bool success = m_simRunner->OnEventStepChanged();
//    bool success = m_simRunner->run();
    bool success = true;

    // 构建执行结果响应
    QJsonObject respObj;
    respObj["id"] = reqId;
    respObj["action"] = "eventStepChanged";
    respObj["errorCode"] = success ? 0 : 1;
    respObj["errorMsg"] = success ? "" : "节拍发送失败";

    QJsonObject data;
    data["eventID"] = eventID;
    data["errorHandle"] = errorHandle;
    data["completed"] = success;
    respObj["data"] = data;

    QJsonDocument doc(respObj);
    QString responseStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // 发送异步执行结果
    QMetaObject::invokeMethod(this, [this, responseStr]() {
        emit asyncResponse(responseStr);
    }, Qt::QueuedConnection);

    // 发送结果数据完整路径
    sendOutputPaths();

    LOG_INFO("节拍异步执行完成: eventID=", eventID,
             " errorHandle=", errorHandle,
             " result=", success ? "成功" : "失败");
}

DDSResponse CommandHandler::handleEventDataRecv(const DDSRequest &request)
{
    DDSResponse ackResponse;
    ackResponse.id = request.id;
    ackResponse.action = request.action;

    if (!m_simRunner || !m_modelLoaded) {
        ackResponse.errorCode = 1;
        ackResponse.errorMsg = "模型未加载";
        return ackResponse;
    }

    // 提取节拍参数
    QString eventID = request.data["eventID"].toString();
    int errorHandle = request.data["errorHandle"].toInt(0);

    //关键： 提取bits数据
    QString bits = request.data["BitsData"].toString();
    std::cout << "CommandHandler::handleEventDataRecv - bits: " << bits.toStdString() << std::endl;
    std::cout << "CommandHandler::handleEventDataRecv - eventID: " << eventID.toStdString() << std::endl;


    // 先回复 ACK（指令已接收）
    ackResponse.errorCode = 0;
//    ackResponse.errorMsg = "节拍指令已接收";
    ackResponse.errorMsg = "";
    ackResponse.data["eventID"] = eventID;
    ackResponse.data["errorHandle"] = errorHandle;

    // 异步执行节拍控制
    QString reqId = request.id;

    QtConcurrent::run([this, eventID, errorHandle, reqId, bits]() {
        executeEventDataRecvAsync(eventID, errorHandle, reqId, bits);
    });

    return ackResponse;  // 立即返回 ACK
}

void CommandHandler::executeEventDataRecvAsync(QString eventID, int errorHandle, const QString &reqId, const QString& bits)
{
    bool success = m_simRunner->OnEventStepChanged(bits);

    // 构建执行结果响应
    QJsonObject respObj;
    respObj["id"] = reqId;
    respObj["action"] = "eventDataRecv";
    respObj["errorCode"] = success ? 0 : 1;
    respObj["errorMsg"] = success ? "" : "节拍发送失败";

    QJsonObject data;
    data["eventID"] = eventID;
    data["errorHandle"] = errorHandle;
    data["completed"] = success;
    respObj["data"] = data;

    QJsonDocument doc(respObj);
    QString responseStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // 发送异步执行结果
    QMetaObject::invokeMethod(this, [this, responseStr]() {
        emit asyncResponse(responseStr);
    }, Qt::QueuedConnection);

    // 发送结果数据完整路径
    sendOutputPaths();

    LOG_INFO("节拍异步执行完成: eventID=", eventID.toStdString(),
             " errorHandle=", errorHandle,
             " result=", success ? "成功" : "失败");
}

DDSResponse CommandHandler::handleCurrTimeChanged(const DDSRequest &request)
{
    DDSResponse response;
    response.id = request.id;
    response.action = response.action;
    if(request.id != "REQ_0009" || request.action != "currTimeChanged") {
        response.errorCode = 1;
        response.errorMsg = "消息类型不匹配";
        return response;
    }

    if(!m_simRunner || !m_modelLoaded) {
        response.errorCode = 1;
        response.errorMsg = "模型未加载";
        return response;
    }

    QString runtime = request.data["runtime"].toString();
    QString runtime_unit = request.data["runtime_unit"].toString();
    QString offset = request.data["offset"].toString();
    QString offset_unit = request.data["offset_unit"].toString();

    response.errorCode = 0;
    response.errorMsg = "";

    return response;
}

DDSResponse CommandHandler::handleVariableParameterChanged(const DDSRequest &request)
{
    DDSResponse response;
    response.id = request.id;
    response.action = response.action;
    if(request.id != "REQ_0010" || request.action != "variableParameterChanged") {
        response.errorCode = 1;
        response.errorMsg = "消息类型不匹配";
        return response;
    }

    if(!m_simRunner || !m_modelLoaded) {
        response.errorCode = 1;
        response.errorMsg = "模型未加载";
        return response;
    }

    response.errorCode = 0;
    response.errorMsg = "";

    return response;
}

void CommandHandler::onEngineLog(const char *level, const char *message, const char *timestamp)
{
    QJsonObject logObj;
    logObj["id"] = "REQ_0020";
    logObj["action"] = "logRecord";
    logObj["errorCode"] = 0;

    QJsonObject data;
    data["level"] = QString::fromUtf8(level);
    data["message"] = QString::fromUtf8(message);
    data["timestamp"] = QString::fromUtf8(timestamp);
    logObj["data"] = data;
    logObj["errorMsg"] = "";

    QJsonDocument doc(logObj);
    QString logStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

//    emit asyncLog(logStr);
//    QMetaObject::invokeMethod(this, [this, logStr]() {
//        emit asyncLog(logStr);
//    }, Qt::QueuedConnection);
}

void CommandHandler::sendOutputPaths()
{
    if (!m_simRunner || !m_modelLoaded) {
        return;
    }

    std::map<std::string, std::string> pathsMap = m_simRunner->GetSinksOutPutPaths();

    QJsonObject msgObj;
    msgObj["id"] = "REQ_0005";
    msgObj["action"] = "sendData";
    msgObj["errorCode"] = 0;

    QJsonObject dataObj;
    QJsonArray pathsArray;
    for (const auto& pair : pathsMap) {
        pathsArray.append(QString::fromStdString(pair.second));
    }
    dataObj["outPutPath"] = pathsArray;
    msgObj["data"] = dataObj;
    msgObj["errorMsg"] = "";

    QJsonDocument doc(msgObj);
    QString message = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

//    emit asyncResponse(message);
    QMetaObject::invokeMethod(this, [this, message]() {
        emit asyncResponse(message);
    }, Qt::QueuedConnection);
}

DDSResponse CommandHandler::createErrorResponse(const QString& id,
                                                 const QString& action,
                                                 int errorCode,
                                                 const QString& errorMsg)
{
    DDSResponse response;
    response.id = id;
    response.action = action;
    response.errorCode = errorCode;
    response.errorMsg = errorMsg;
    return response;
}

QString CommandHandler::getStateString(int state)
{
    switch (state) {
        case 0: return "初始化完成";
        case 1: return "运行中";
        case 2: return "暂停";
        case 3: return "停止";
        default: return "未知状态";
    }
}
