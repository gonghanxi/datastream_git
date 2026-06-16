// LinkParser.cpp
#include "LinkParser.h"
#include "FMUModelInfo.h"
#include "CFunctionModelInfo.h"
#include <QFile>
#include <QDebug>

QJsonDocument LinkParser::readJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open the file:" << filePath;
        return QJsonDocument();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    return QJsonDocument::fromJson(jsonData);
}

QString LinkParser::extractId(const QString& id, const QString& prefix)
{
    if (id.startsWith(prefix)) {
        return id.mid(prefix.length());
    }
    return id;
}

SimuParameter LinkParser::getSimuParameterByLinkKey(
    const QString& linkKey,
    const QMap<QString, SimuParameter>& simuParams,
    const QString& mainLinkKey)
{
    if (simuParams.contains(linkKey)) {
        return simuParams[linkKey];
    }
    // 子链路使用主链路的仿真参数
    return simuParams.value(mainLinkKey);
}

// ========== 第一阶段：收集和注册 ==========
bool LinkParser::parseSimuParameters(
    const QJsonObject& firstObj,
    const QString& mainLinkKey,
    QMap<QString, SimuParameter>& simuParams)
{
    if (!firstObj.contains("simuParams") || !firstObj["simuParams"].isObject()) {
        LOG_ERROR("仿真器参数缺失.");
        return false;
    }

    SimuParameter simuPara;
    auto simuObj = firstObj["simuParams"].toObject();

    // 读取链路名
    if (firstObj.contains("name") && firstObj["name"].isString()) {
        simuPara.linkName = firstObj["name"].toString().toStdString();
    }

    QString startTime_Value = simuObj["StartTime"].toString();
    QString startTime_Unit = simuObj["StartTime_Unit"].toString();
    QString stopTime_Value = simuObj["StopTime"].toString();
    QString stopTime_Unit = simuObj["StopTime_Unit"].toString();
    QString samplingRate_Value = simuObj["SamplingRate"].toString();
    QString samplingRate_Unit = simuObj["SamplingRate_Unit"].toString();
    QString time_Interval_Value = simuObj["Time_Interval"].toString();
    QString time_Interval_Unit = simuObj["Time_Interval_Unit"].toString();

    simuPara.simuName = simuObj["simuName"].toString().toStdString();

    bool ok = true;
    simuPara.startTime = UnitConvert::convertToStandardUnit("time", startTime_Unit, startTime_Value).toDouble(&ok);
    double stopTime = UnitConvert::convertToStandardUnit("time", stopTime_Unit, stopTime_Value).toDouble(&ok);
    simuPara.stopTime = stopTime;
    double sampleRate = UnitConvert::convertToStandardUnit("frequency", samplingRate_Unit, samplingRate_Value).toDouble(&ok);
//    sampleRate *= 2;
    simuPara.samplingRate = sampleRate;
    double time_interval = UnitConvert::convertToStandardUnit("time", time_Interval_Unit, time_Interval_Value).toDouble(&ok);
//    time_interval /= 2;
    simuPara.time_Interval = time_interval;
    int num_Samples = simuObj["Num_Samples"].toString().toInt(&ok);
    simuPara.num_Samples = num_Samples;

    qDebug() << "仿真次数：" << simuPara.num_Samples;
    qDebug() << "stopTime: " << simuPara.stopTime;
    qDebug() << "time_Interval: " << simuPara.time_Interval;
    qDebug() << "num_Samples: " << simuPara.num_Samples;
    qDebug() << "samplingRate:" << simuPara.samplingRate;

    if (!ok) {
        LOG_ERROR("仿真器参数错误.");
        return false;
    }

    simuParams[mainLinkKey] = simuPara;
    return true;
}

bool LinkParser::parseVariables(
    const QJsonObject& jsonObj,
    const QString& linkKey,
    QMap<QString, QVector<Variable>>& allVariables)
{
    if (!jsonObj.contains("vars") || !jsonObj["vars"].isArray()) {
        return true; // 没有变量也是合法的
    }

    QJsonArray varsArray = jsonObj["vars"].toArray();
    QVector<Variable> variables;

    for (const QJsonValue& varValue : varsArray) {
        if (!varValue.isObject()) continue;

        QJsonObject varObj = varValue.toObject();
        Variable var = Variable::fromJson(varObj);
        variables.append(var);

        qDebug() << "收集变量: 链路=" << linkKey
                 << ", 变量=" << var.name
                 << ", 默认值=" << var.defaultValue;
    }

    if (!variables.isEmpty()) {
        allVariables[linkKey] = variables;
    }

    return true;
}

bool LinkParser::collectLinkInfo(
    const QString& linkFile,
    QMap<QString, QJsonObject>& linkObjects,
    QMap<QString, QString>& parentLinkMap,
    QString& mainLinkKey,
    QMap<QString, SimuParameter>& simuParams,
    QMap<QString, QVector<Variable>>& allVariables)
{
    QJsonDocument jsonDoc = readJsonFile(linkFile);
    if (jsonDoc.isNull()) {
        LOG_ERROR("链路文件解析失败:", linkFile.toStdString());
        return false;
    }

    if (!jsonDoc.isArray()) {
        LOG_ERROR("链路文件格式错误: 不是数组", linkFile.toStdString());
        return false;
    }

    QJsonArray jsonArray = jsonDoc.array();

    // 获取主链路信息
    if (!jsonArray.first().isObject()) {
        LOG_ERROR("链路文件格式错误: 第一个元素不是对象");
        return false;
    }

    QJsonObject firstObj = jsonArray.first().toObject();

    if (!firstObj.contains("linkkey") || !firstObj["linkkey"].isString()) {
        LOG_ERROR("链路文件：", linkFile.toStdString(), "主链路linkkey不存在");
        return false;
    }

    mainLinkKey = firstObj["linkkey"].toString();

    // 解析仿真参数
    if (!parseSimuParameters(firstObj, mainLinkKey, simuParams)) {
        return false;
    }

    // 遍历所有链路
    for (const QJsonValue& value : jsonArray) {
        if (!value.isObject()) continue;

        QJsonObject jsonObj = value.toObject();
        QString linkKey = jsonObj["linkkey"].toString();

        if (linkKey.isEmpty()) {
            LOG_ERROR("链路文件：", linkFile.toStdString(), "linkkey不存在");
            return false;
        }

        // 存储JSON对象
        linkObjects[linkKey] = jsonObj;

        // 确定父作用域
        if (linkKey != mainLinkKey) {
            parentLinkMap[linkKey] = mainLinkKey;
        }

        // 解析变量
        if (!parseVariables(jsonObj, linkKey, allVariables)) {
            return false;
        }
    }

    return true;
}

// ========== 第二阶段：设置作用域 ==========
void LinkParser::setupScopes(
    VariableScopeManager& scopeMgr,
    const QMap<QString, QVector<Variable>>& allVariables)
{
    // 先注册所有作用域
    for (auto it = allVariables.begin(); it != allVariables.end(); ++it) {
        const QString& linkKey = it.key();
        if (!scopeMgr.hasScope(linkKey)) {
            scopeMgr.registerScope(linkKey);
        }
    }

    // 再设置变量
    for (auto it = allVariables.begin(); it != allVariables.end(); ++it) {
        const QString& linkKey = it.key();
        const QVector<Variable>& variables = it.value();

        if (!variables.isEmpty()) {
            scopeMgr.setScopeVariables(linkKey, variables);
            qDebug() << "设置作用域变量:" << linkKey << "变量数:" << variables.size();
        }
    }
}

// ========== 第三阶段：处理子系统映射 ==========
bool LinkParser::processSubsystemMapping(
    SubsystemParameterMapper& paramMapper,
    const QMap<QString, QJsonObject>& linkObjects,
    const QMap<QString, QString>& parentLinkMap)
{
    for (auto it = linkObjects.begin(); it != linkObjects.end(); ++it) {
        QString parentScopeId = it.key();
        QJsonObject obj = it.value();

        if (!obj.contains("cmpSet") || !obj["cmpSet"].isArray()) {
            continue;
        }
        if(parentLinkMap.isEmpty()) {

        }

        QJsonArray cmpArray = obj["cmpSet"].toArray();

        for (const QJsonValue& cmpValue : cmpArray) {
            if (!cmpValue.isObject()) continue;

            QJsonObject cmpObj = cmpValue.toObject();

            bool isSubSystem = cmpObj["isSubSystem"].toBool() ||
                              cmpObj["objectType"].toString() == "subSystem";

            if (isSubSystem) {
                QString childTopoId = cmpObj["childTopoId"].toString();
                if (!childTopoId.isEmpty()) {
                    qDebug() << "发现子系统组件:" << cmpObj["instanceName"].toString()
                             << "父链路:" << parentScopeId
                             << "子链路:" << childTopoId;

                    // 这里会在映射的同时设置参数覆盖值
                    if (!paramMapper.mapSubsystemParameters(parentScopeId, cmpObj, childTopoId)) {
                        LOG_ERROR("子系统参数映射失败:", cmpObj["instanceName"].toString().toStdString());
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

// ========== 第四阶段：解析模型和参数 ==========
bool LinkParser::parsePorts(
    const QJsonObject& cmpObj,
    BlockInfo& blockInfo,
    bool& topProtIdExist)
{
    if (!cmpObj.contains("port") || !cmpObj["port"].isArray()) {
        return true;
    }
    qDebug() << "========== parsePorts begin ==========";

    auto portArray = cmpObj["port"].toArray();

    for (const QJsonValue& portValue : portArray) {
        if (!portValue.isObject()) continue;

        auto portObj = portValue.toObject();
        QString portId = extractId(portObj["id"].toString(), "p_");
        QString portPutType = portObj["putType"].toString();
        QString portName = portObj["name"].toString();
        QString portDataType = portObj["dataType"].toString();
        QString portRate = portObj["PortRate"].toString();
        bool portisOptional = portObj["isOptional"].toBool();

        qDebug() << QString("端口ID:%1,数据类型:%2,方向:%3,名称:%4,速率:%5")
                    .arg(portId).arg(portDataType).arg(portPutType).arg(portName).arg(portRate);

        PortMsg port;
        port.id = portId.toInt();
        port.putType = portPutType;
        port.name = portName;
        port.isOptional = portisOptional;
        port.dataType = UnitConvert::convertToDataType(portDataType);
        port.portRate=std::atoi(portRate.toStdString().c_str());
        qDebug() << "port.portRate: " << port.portRate;
        port.portRate=std::atoi(portRate.toStdString().c_str());
        if (portObj.contains("topProtId")) {
            port.topProtId = extractId(portObj["topProtId"].toString(), "p_").toInt();
            topProtIdExist = true;
        } else {
            port.topProtId = -1;
        }

        blockInfo.portsMsg[port.id] = port;
        qDebug() << "parsePorts -- portsMsg size: " << blockInfo.portsMsg.size();
    }

    return true;
}

bool LinkParser::parseParameters(
    const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    BlockInfo& blockInfo,
    const QVector<Variable>& currentVars,
    VarExpressionParse& varParser,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver)
{
    std::ignore = scopeMgr;
    if (!cmpObj.contains("attribute") || !cmpObj["attribute"].isArray()) {
        return true;
    }
    qDebug() << "========== parseParameters begin ==========";

    auto paramArray = cmpObj["attribute"].toArray();

    for (const QJsonValue& paramValue : paramArray) {
        if (!paramValue.isObject()) continue;

        auto paramObj = paramValue.toObject();

        QString paramName = paramObj["name"].toString();
        QString paramDataType = paramObj["dataType"].toString();
        QString paramUnitType = paramObj["unitType"].toString();
        QString paramUnit = paramObj["unit"].toString();
        QString paramVal = paramObj["value"].toString();

        // ===== 新增：处理默认值逻辑 =====
        bool isDefault = paramObj["isDefault"].toBool(false);  // 第二个参数是默认值，如果字段不存在返回false

        if (isDefault) {
            QString defaultVal;

            // 优先使用 valDefault
            if (paramObj.contains("valDefault")) {
                QJsonValue valDefault = paramObj["valDefault"];
                if (valDefault.isDouble()) {
                    defaultVal = QString::number(valDefault.toDouble());
                } else if (valDefault.isString()) {
                    defaultVal = valDefault.toString();
                } else if (valDefault.isBool()) {
                    defaultVal = valDefault.toBool() ? "true" : "false";
                } else if (!valDefault.isNull() && !valDefault.isUndefined()) {
                    defaultVal = valDefault.toString();
                }
            }
            // 其次使用 defaultValue
//            else if (paramObj.contains("defaultValue")) {
//                QJsonValue defaultValue = paramObj["defaultValue"];
//                if (defaultValue.isDouble()) {
//                    defaultVal = QString::number(defaultValue.toDouble());
//                } else if (defaultValue.isString()) {
//                    defaultVal = defaultValue.toString();
//                } else if (defaultValue.isBool()) {
//                    defaultVal = defaultValue.toBool() ? "true" : "false";
//                } else if (!defaultValue.isNull() && !defaultValue.isUndefined()) {
//                    defaultVal = defaultValue.toString();
//                }
//            }

            // 如果找到了默认值，替换 paramVal
            if (!defaultVal.isEmpty()) {
                qDebug() << "使用默认值 (isDefault=true):" << paramName
                         << "原始值:" << paramVal
                         << "默认值:" << defaultVal;
                paramVal = defaultVal;
            } else {
                qDebug() << "警告: isDefault=true 但未找到 valDefault 或 defaultValue 字段";
            }
        }
        // ===== 新增逻辑结束 =====

        // ===== 完整跳过inPort/outPort模型的所有参数处理 =====
        if (blockInfo.cmpType == "inPort" || blockInfo.cmpType == "outPort") {
            qDebug() << "跳过子系统出入口模型参数:"
                     << "模型:" << blockInfo.instanceName
                     << "参数:" << paramName << "=" << paramVal;
            continue;  // 跳过后续所有处理
        }

        // 获取calculateValue
        QString calculateValueStr;
        if (paramObj.contains("calculateValue")) {

            QJsonValue calculateValue = paramObj["calculateValue"];
            if (calculateValue.isDouble()) {
                calculateValueStr = QString::number(calculateValue.toDouble());

            } else if (calculateValue.isString()) {
                calculateValueStr = calculateValue.toString();
            } else if (calculateValue.isBool()) {
                calculateValueStr = calculateValue.toBool() ? "true" : "false";
            } else if (!calculateValue.isNull() && !calculateValue.isUndefined()) {
                calculateValueStr = calculateValue.toString();
            }
        }

        QString originalParamVal = paramVal;
        QString finalParamVal = paramVal;

        // 使用ExpressionResolver解析
        ResolutionResult resolveResult = resolver.resolveExpression(
            currentLinkKey, paramVal, calculateValueStr);

        if (resolveResult.success) {
            finalParamVal = resolveResult.value;
            paramVal = finalParamVal;
            qDebug() << "ExpressionResolver解析成功:"
                     << originalParamVal << "->" << finalParamVal;
        } else {
            // Fallback to old parser
            if (!currentVars.isEmpty()) {
                varParser.setVariables(currentVars);
            }

            if (varParser.isArrayExpression(paramVal)) {
                ExpressionResult parseResult = varParser.parseArrayWithExpressions(paramVal);
                if (parseResult.success) {
                    finalParamVal = parseResult.value;
                    paramVal = finalParamVal;
                }
            } else {
                ExpressionResult parseResult = varParser.parseExpression(paramVal);
                if (parseResult.success) {
                    finalParamVal = parseResult.value;
                    paramVal = finalParamVal;
                } else if (!calculateValueStr.isEmpty()) {
                    paramVal = calculateValueStr;
                }
            }
        }

        // ===== 参数校验（只对非inPort/outPort、子系统模型执行）=====

        if(blockInfo.cmpType != "subSystem") {
            // 枚举类型校验
            if (paramDataType.toLower() == "enumeration") {
                if (paramObj.contains("selectOptions") && paramObj["selectOptions"].isArray()) {
                    QJsonArray selectOptions = paramObj["selectOptions"].toArray();
                    QString enumError = UnitConvert::validateEnumeration(paramVal, selectOptions);
                    if (!enumError.isEmpty()) {
                        LOG_ERROR("链路：", currentLinkKey.toStdString(),
                                 "，实例：", blockInfo.instanceName.toStdString(),
                                 "，参数：", paramName.toStdString(), " - ", enumError.toStdString());
                        return false;
                    }
                }
            }
            // 数组类型校验
            else if (paramDataType.toLower().contains("array") || paramVal.startsWith('[')) {
                if (!UnitConvert::isValidArrayFormat(paramVal)) {
                    LOG_ERROR("链路：", currentLinkKey.toStdString(),
                              "，实例：", blockInfo.instanceName.toStdString(),
                              "，参数：", paramName.toStdString(), " - 无效的数组格式");
                    return false;
                }

                if (paramUnitType.toUpper() != "NONE") {
                    QStringList elements = UnitConvert::parseArrayElements(paramVal);
                    for (int i = 0; i < elements.size(); i++) {
                        bool isNumber = false;
                        elements[i].toDouble(&isNumber);
                        if (!isNumber) {
                            LOG_ERROR("链路：", currentLinkKey.toStdString(),
                                      "，实例：", blockInfo.instanceName.toStdString(),
                                      "，参数：", paramName.toStdString(),
                                      " - 数组元素[", std::to_string(i), "]不是有效的数值");
                            return false;
                        }
                    }
                }
            }
            // 其他数据类型校验
            else {
                QString typeError = UnitConvert::validateParameterType(paramDataType, paramVal);
                if (!typeError.isEmpty()) {
                    // 复数特殊处理
                    if (paramDataType.toLower() == "complex") {
                        bool isNumber = false;
                        paramVal.toDouble(&isNumber);
                        if (!isNumber) {
                            LOG_ERROR("链路：", currentLinkKey.toStdString(),
                                     "，实例：", blockInfo.instanceName.toStdString(),
                                     "，参数：", paramName.toStdString(), " - ", typeError.toStdString());
                            return false;
                        }
                    } else {
                        LOG_ERROR("链路：", currentLinkKey.toStdString(),
                                 "，实例：", blockInfo.instanceName.toStdString(),
                                 "，参数：", paramName.toStdString(), " - ", typeError.toStdString());
                        return false;
                    }
                }

                // 数组一致性检查
                QString arrayError = UnitConvert::validateArrayConsistency(paramDataType, paramVal);
                if (!arrayError.isEmpty()) {
                    LOG_ERROR("链路：", currentLinkKey.toStdString(),
                             "，实例：", blockInfo.instanceName.toStdString(),
                             "，参数：", paramName.toStdString(), " - ", arrayError.toStdString());
                    return false;
                }
            }
        }

        // 单位转换
        QString paramVal_convert = UnitConvert::convertToStandardUnit(
            paramUnitType, paramUnit, paramDataType, paramVal);

        if (paramVal_convert.contains("invalid_value")) {
            LOG_ERROR("链路：", currentLinkKey.toStdString(),
                     "，实例：", blockInfo.instanceName.toStdString(),
                     "，参数：", paramName.toStdString(), "单位转换错误.");
            return false;
        }

        // 创建Parameter（只有Name和Value）
        Parameter parameter;
        parameter.Name = paramName.toStdString();
        parameter.Value = paramVal_convert.toStdString();

        blockInfo.parameters[parameter.Name] = parameter;

        qDebug() << QString("参数:%1,转换值:%2").arg(paramName).arg(paramVal_convert);
    }

    return true;
}
bool LinkParser::parseSingleModel(const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QString& appPath,
    const QString& outPutPath,
    const SimuParameter& simuPara,
    const QVector<Variable>& currentVars,
    VarExpressionParse& varParser,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    QVector<BlockInfo>& blocksInfo)
{
    BlockInfo blockInfo;
    blockInfo.cmpId = extractId(cmpObj["cmpId"].toString(), "cp_").toInt();
    blockInfo.isSubSystem = cmpObj["isSubSystem"].toBool();
    blockInfo.cmpType = cmpObj["cmpType"].toString();
    blockInfo.instanceName = cmpObj["instanceName"].toString();
    blockInfo.childTopoId = cmpObj["childTopoId"].toString();
    blockInfo.cmpCondition = cmpObj["cmpCondition"].toString();

    qDebug() << "解析到 cmpCondition:" << blockInfo.cmpCondition
             << "模型:" << blockInfo.instanceName;
    // 解析 cmpCategory（模型类别）
    if (cmpObj.contains("cmpCategory") && cmpObj["cmpCategory"].isArray()) {
        QJsonArray categoryArray = cmpObj["cmpCategory"].toArray();
        if (!categoryArray.isEmpty()) {
            // 取第一个类别（通常只有一个）
            blockInfo.cmpCategory = categoryArray.first().toString();
            qDebug() << "解析到 cmpCategory:" << blockInfo.cmpCategory
                     << "模型:" << blockInfo.instanceName;
        } else {
            blockInfo.cmpCategory = "";
        }
    } else {
        blockInfo.cmpCategory = "";
    }

    // 计算子系统路径
    QString subsystemPath;
    if (!m_subsystemPathStack.isEmpty()) {
        subsystemPath = m_subsystemPathStack.join("/");
        blockInfo.subsystemPath = subsystemPath;
    }

    qDebug() << "========== parseSingleModel begin ==========";
    qDebug() << QString("实例:%1,类型:%2,ID:%3,子系统:%4,子系统路径:%5")
                .arg(blockInfo.instanceName)
                .arg(blockInfo.cmpType)
                .arg(blockInfo.cmpId)
                .arg(blockInfo.isSubSystem)
                .arg(subsystemPath);

    // 判断是否为FMU模型
    QString cmpType = cmpObj["cmpType"].toString();
    if (cmpType == "Fmu") {
        // 使用独立的FMU解析器
        FMUModelParser fmuParser;
        FMUModelInfo fmuModelInfo;

        if (!fmuParser.parseFMUModel(currentLinkKey, cmpObj,
                                     currentVars,simuPara,
                                     scopeMgr, resolver, fmuModelInfo)) {
            LOG_ERROR("FMU模型解析失败:", fmuModelInfo.instanceName.toStdString());
            return false;
        }

        // 转换为BlockInfo并添加到结果
        BlockInfo blockInfo = fmuModelInfo.toBlockInfo();

        // 设置子系统路径
        if (!m_subsystemPathStack.isEmpty()) {
            blockInfo.subsystemPath = m_subsystemPathStack.join("/");
        }

        blocksInfo.append(blockInfo);

        qDebug() << "FMU模型添加成功:" << blockInfo.instanceName
                 << "GUID:" << blockInfo.guid
                 << "端口数:" << blockInfo.portsMsg.size()
                 << "参数数:" << blockInfo.parameters.size();

        return true;
    }

    // 判断是否为CFunction模型
    if (cmpType == "CFunction") {
        CFunctionModelParser cfuncParser;
        CFunctionModelInfo cfuncModelInfo;

        if (!cfuncParser.parseCFunctionModel(currentLinkKey, cmpObj,
                                             currentVars, simuPara,
                                             scopeMgr, resolver, cfuncModelInfo)) {
            LOG_ERROR("CFunction模型解析失败:", cfuncModelInfo.instanceName.toStdString());
            return false;
        }

        // 生成cfunction.json文件：存储到 appPath/{linkkey}/ 目录下
        // 文件名使用instanceName，即 {instanceName}.json
        QString cfunctionOutputDir = appPath + "/" + currentLinkKey;
        QString generatedPath = cfuncParser.generateCFunctionJson(cfuncModelInfo, cfunctionOutputDir);
        if (generatedPath.isEmpty()) {
            LOG_ERROR("CFunction JSON生成失败:", cfuncModelInfo.instanceName.toStdString());
            return false;
        }
        cfuncModelInfo.generatedJsonPath = generatedPath;

        // 转换为BlockInfo并添加到结果
        BlockInfo blockInfo = cfuncModelInfo.toBlockInfo();

        // 设置子系统路径
        if (!m_subsystemPathStack.isEmpty()) {
            blockInfo.subsystemPath = m_subsystemPathStack.join("/");
        }

        blocksInfo.append(blockInfo);

        qDebug() << "CFunction模型添加成功:" << blockInfo.instanceName
                 << "端口数:" << blockInfo.portsMsg.size()
                 << "参数数:" << blockInfo.parameters.size()
                 << "JSON路径:" << generatedPath;

        return true;
    }

    // 如果是子系统，压入路径栈
    bool isSubSystem = blockInfo.isSubSystem;
    if (isSubSystem) {
        m_subsystemPathStack.push_back(blockInfo.instanceName);
        qDebug() << "进入子系统，当前路径栈:" << m_subsystemPathStack;
    }

    // 解析端口
    bool topProtIdExist = false;
    if (!parsePorts(cmpObj, blockInfo, topProtIdExist)) {
        return false;
    }

    // 解析参数
    if (!parseParameters(currentLinkKey, cmpObj, blockInfo, currentVars,
                         varParser, scopeMgr, resolver)) {
        return false;
    }

    // ===== 添加Equations字段解析 =====
    if (blockInfo.cmpType == "MATLAB_Script") {
        if(cmpObj.contains("Equations")) {
            Parameter para;
            para.Name = "Equations";
            para.Value = cmpObj["Equations"].toString().toStdString();
            blockInfo.parameters["Equations"] = para;
            qDebug() << "添加Equations字段:" << blockInfo.instanceName
                     << "内容:" << para.Value.c_str();
        }
    }

    // 创建模型实例
    if (!blockInfo.isSubSystem && !topProtIdExist) {
        auto algo = AlgorithmManager::createInstance()->getAlgorithm(
            appPath, blockInfo.cmpType, blockInfo.instanceName);

        if (!algo) {
            LOG_ERROR("链路：", currentLinkKey.toStdString(),
                     "，实例：", blockInfo.instanceName.toStdString(), "创建失败.");
            return false;
        }

        // ===== 转换并设置端口信息 =====
        QMap<int, PortMsg> algoPorts;
        for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
            const PortMsg& srcPort = it.value();
            PortMsg dstPort;

            // 复制字段
            dstPort.id = srcPort.id;
            dstPort.putType = srcPort.putType;
            dstPort.name = srcPort.name;
            dstPort.dataType = static_cast<PortMsg::PortDataType>(srcPort.dataType);
            dstPort.topProtId = srcPort.topProtId;
            dstPort.isOptional = srcPort.isOptional;
            dstPort.portRate = srcPort.portRate;

            algoPorts[it.key()] = dstPort;
        }
        algo->setPortsMsg(algoPorts);

        algo->setId(blockInfo.cmpId);

        // 创建仿真参数的副本
        SimuParameter updatedSimuPara = simuPara;

        // 如果有子系统路径，设置最后一个作为当前子系统名称
        if (!m_subsystemPathStack.isEmpty()) {
            QString currentSubsystemName = m_subsystemPathStack.last();
            updatedSimuPara.subsystemName = currentSubsystemName.toStdString();
            qDebug() << "设置子系统名称:" << currentSubsystemName
                     << "到实例:" << blockInfo.instanceName
                     << "完整路径:" << m_subsystemPathStack;
        }

        algo->setSimuParams(updatedSimuPara);

        // 设置完整路径到block（如果需要）
        if (!m_subsystemPathStack.isEmpty()) {
            algo->setSubsystemName(m_subsystemPathStack.last().toStdString());
        }

        std::string utf8Path = outPutPath.toUtf8().constData();
        algo->setOutPutPath(utf8Path);

        blockInfo.block = algo;
    } else {
        blockInfo.block = nullptr;
    }

    blocksInfo.append(blockInfo);

    // 如果是子系统，解析完内部模型后弹出路径栈
    if (isSubSystem) {
        // 递归解析子系统内部的模型
        if (cmpObj.contains("childJson") && cmpObj["childJson"].isArray()) {
            QJsonArray childJsonArray = cmpObj["childJson"].toArray();
            for (const QJsonValue& childValue : childJsonArray) {
                if (!childValue.isObject()) continue;

                QJsonObject childObj = childValue.toObject();
            }
        }

        m_subsystemPathStack.pop_back();
        qDebug() << "退出子系统，当前路径栈:" << m_subsystemPathStack;
    }

    if (blockInfo.isSubSystem) {
        // 解析子系统组件的参数
        if (!parseParameters(currentLinkKey, cmpObj, blockInfo, currentVars,
                             varParser, scopeMgr, resolver)) {
            return false;
        }
    }

    return true;
}

bool LinkParser::parseModelsAndParameters(
    const QString& appPath,
    const QString& outPutPath,
    const QMap<QString, QJsonObject>& linkObjects,
    const QMap<QString, SimuParameter>& simuParams,
    const QMap<QString, QVector<Variable>>& allVariables,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
    QMap<QString, QVector<Connection>>& allConnections)
{
    QString mainLinkKey = simuParams.keys().first(); // 主链路

    for (auto it = linkObjects.begin(); it != linkObjects.end(); ++it) {
        QString currentLinkKey = it.key();
        QJsonObject jsonObj = it.value();

        qDebug() << "\n========== 解析链路:" << currentLinkKey << "==========";

        // 获取当前链路的变量
        QVector<Variable> currentVars = allVariables.value(currentLinkKey);
        VarExpressionParse varParser; // 兼容旧解析器
        if (!currentVars.isEmpty()) {
            varParser.setVariables(currentVars);
        }

        // 获取仿真参数
        SimuParameter simuPara = getSimuParameterByLinkKey(
            currentLinkKey, simuParams, mainLinkKey);

        QVector<BlockInfo> blocksInfo;

        // 清空子系统路径栈（每个新链路开始时）
        m_subsystemPathStack.clear();

        // 解析cmpSet
        if (jsonObj.contains("cmpSet") && jsonObj["cmpSet"].isArray()) {
            auto cmpArray = jsonObj["cmpSet"].toArray();

            for (const QJsonValue& cmpValue : cmpArray) {
                if (!cmpValue.isObject()) continue;

                QJsonObject cmpObj = cmpValue.toObject();
                QString objectType = cmpObj["objectType"].toString();

                if (objectType == "model" || objectType == "subSystem") {
                    if (!parseSingleModel(currentLinkKey, cmpObj, appPath, outPutPath,
                                         simuPara, currentVars, varParser,
                                         scopeMgr, resolver, blocksInfo)) {
                        return false;
                    }
                }
            }
        }

        if (!blocksInfo.isEmpty()) {
            allBlocksInfo[currentLinkKey] = blocksInfo;
        }

        // 解析连接关系
        if (!parseConnections(jsonObj, currentLinkKey, allConnections)) {
            return false;
        }
    }

    return true;
}

// ========== 第五阶段：解析连接关系 ==========
bool LinkParser::parseConnections(
    const QJsonObject& jsonObj,
    const QString& currentLinkKey,
    QMap<QString, QVector<Connection>>& allConnections)
{
    if (!jsonObj.contains("ConnectSet") || !jsonObj["ConnectSet"].isArray()) {
        return true;
    }

    auto lineArray = jsonObj["ConnectSet"].toArray();
    QVector<Connection> connections;

    for (const QJsonValue& lineValue : lineArray) {
        if (!lineValue.isObject()) continue;

        auto lineObj = lineValue.toObject();
        QString cmpIdStart = extractId(lineObj["cmpIdStart"].toString(), "cp_");
        QString cmpIdEnd = extractId(lineObj["cmpIdEnd"].toString(), "cp_");
        QString portIdStart = extractId(lineObj["portIdStart"].toString(), "p_");
        QString portIdEnd = extractId(lineObj["portIdEnd"].toString(), "p_");

        qDebug() << QString("连接:源组件:%1,目标组件:%2,源端口:%3,目标端口:%4")
                    .arg(cmpIdStart).arg(cmpIdEnd).arg(portIdStart).arg(portIdEnd);

        Connection conn(cmpIdStart, portIdStart, cmpIdEnd, portIdEnd);
        connections.append(conn);
    }

    if (!connections.isEmpty()) {
        allConnections[currentLinkKey] = connections;
    }

    return true;
}

// ========== 辅助方法 ==========
bool LinkParser::dataCollectionCheck(const QMap<QString, QVector<BlockInfo>>& allBlocksInfo)
{
    QStringList DataCollection = {"Sink", "SpectrumAnalyzer"};

    for (auto it = allBlocksInfo.begin(); it != allBlocksInfo.end(); ++it) {
        bool exit = false;
        const QString& linkKey = it.key();
        const QVector<BlockInfo>& blocksInfo = it.value();

        for (const auto& blockInfo : blocksInfo) {
            if (DataCollection.contains(blockInfo.cmpType)) {
                exit = true;
                break;
            }
        }

        if (!exit) {
            LOG_ERROR("链路：", linkKey.toStdString(), "，没有用于收集数据的模型。模拟无法运行。");
            return false;
        }
    }

    return true;
}

// ========== 主解析接口 ==========
ParseResult LinkParser::parseLinkFiles(
    const QString& appPath,
    const QStringList& linkFiles,
    const QString& outPutPath)
{
    ParseResult result;
    result.success = false;

    // 数据容器
    QMap<QString, QJsonObject> linkObjects;
    QMap<QString, QString> parentLinkMap;
    QString mainLinkKey;
    QMap<QString, SimuParameter> simuParams;
    QMap<QString, QVector<Variable>> allVariables;
    QMap<QString, QVector<BlockInfo>> allBlocksInfo;
    QMap<QString, QVector<Connection>> allConnections;

    // 创建核心组件
    VariableScopeManager scopeMgr;
    SubsystemParameterMapper paramMapper(&scopeMgr);
    ExpressionResolver resolver(&scopeMgr);
    MathExpressionCalculator mathCalc;

    // ===== 第一阶段：收集和注册 =====
    // 先收集所有信息，但不解析模型
    for (const QString& linkFile : linkFiles) {
        if (!collectLinkInfo(linkFile, linkObjects, parentLinkMap, mainLinkKey,
                            simuParams, allVariables)) {
            result.errorMessage = "收集链路信息失败";
            return result;
        }
    }

    // ===== 第二阶段：设置作用域 =====
    for (auto it = parentLinkMap.begin(); it != parentLinkMap.end(); ++it) {
        scopeMgr.registerScope(it.key(), it.value());
    }
    setupScopes(scopeMgr, allVariables);

    // ===== 第三阶段：处理子系统映射 =====
    if (!processSubsystemMapping(paramMapper, linkObjects, parentLinkMap)) {
        result.errorMessage = "处理子系统映射失败";
        return result;
    }

    // ===== 第四阶段：解析模型和参数 =====
    // 创建一个映射：childTopoId -> parentSubsystemName
    QMap<QString, QString> childTopoIdToSubsystemName;

    // 首先收集所有子系统的映射关系
    for (auto it = linkObjects.begin(); it != linkObjects.end(); ++it) {
        QJsonObject jsonObj = it.value();

        if (jsonObj.contains("cmpSet") && jsonObj["cmpSet"].isArray()) {
            auto cmpArray = jsonObj["cmpSet"].toArray();
            for (const QJsonValue& cmpValue : cmpArray) {
                if (!cmpValue.isObject()) continue;
                QJsonObject cmpObj = cmpValue.toObject();

                bool isSubSystem = cmpObj["isSubSystem"].toBool() ||
                                  cmpObj["objectType"].toString() == "subSystem";

                if (isSubSystem && cmpObj.contains("childTopoId")) {
                    QString childTopoId = cmpObj["childTopoId"].toString();
                    QString subsystemName = cmpObj["instanceName"].toString();
                    childTopoIdToSubsystemName[childTopoId] = subsystemName;
                    qDebug() << "记录子系统映射:" << childTopoId << "->" << subsystemName;
                }
            }
        }
    }

    // 按正确顺序解析：先解析子链路，再解析父链路
    // 找出所有链路，并按依赖关系排序
    QSet<QString> processed;

    // 先解析所有子链路（非主链路），最后解析主链路
    for (auto it = linkObjects.begin(); it != linkObjects.end(); ++it) {
        if (it.key() != mainLinkKey) {
            // 解析子链路
            QString currentLinkKey = it.key();
            QJsonObject jsonObj = it.value();

            // 获取当前链路的子系统名称（如果有）
            QString subsystemName = childTopoIdToSubsystemName.value(currentLinkKey, "");

            qDebug() << "\n========== 解析子链路:" << currentLinkKey
                     << "，所属子系统:" << subsystemName << "==========";

            // 设置当前子系统路径
            m_subsystemPathStack.clear();
            if (!subsystemName.isEmpty()) {
                m_subsystemPathStack.push_back(subsystemName);
            }

            // 获取当前链路的变量和仿真参数
            QVector<Variable> currentVars = allVariables.value(currentLinkKey);
            SimuParameter simuPara = getSimuParameterByLinkKey(
                currentLinkKey, simuParams, mainLinkKey);

            QVector<BlockInfo> blocksInfo;

            // 解析cmpSet
            if (jsonObj.contains("cmpSet") && jsonObj["cmpSet"].isArray()) {
                auto cmpArray = jsonObj["cmpSet"].toArray();
                for (const QJsonValue& cmpValue : cmpArray) {
                    if (!cmpValue.isObject()) continue;
                    QJsonObject cmpObj = cmpValue.toObject();
                    QString objectType = cmpObj["objectType"].toString();

                    if (objectType == "model" || objectType == "subSystem") {
                        VarExpressionParse varParser;
                        if (!currentVars.isEmpty()) {
                            varParser.setVariables(currentVars);
                        }

                        if (!parseSingleModel(currentLinkKey, cmpObj, appPath, outPutPath,
                                             simuPara, currentVars, varParser,
                                             scopeMgr, resolver, blocksInfo)) {
                            result.errorMessage = "解析模型和参数失败";
                            return result;
                        }
                    }
                }
            }

            if (!blocksInfo.isEmpty()) {
                allBlocksInfo[currentLinkKey] = blocksInfo;
            }

            // 解析连接关系
            if (!parseConnections(jsonObj, currentLinkKey, allConnections)) {
                result.errorMessage = "解析连接关系失败";
                return result;
            }

            processed.insert(currentLinkKey);
        }
    }

    // 最后解析主链路
    qDebug() << "\n========== 解析主链路:" << mainLinkKey << "==========";
    m_subsystemPathStack.clear();  // 主链路没有子系统路径

    QJsonObject mainJsonObj = linkObjects[mainLinkKey];
    QVector<Variable> mainVars = allVariables.value(mainLinkKey);
    SimuParameter mainSimuPara = simuParams[mainLinkKey];

    QVector<BlockInfo> mainBlocksInfo;

    if (mainJsonObj.contains("cmpSet") && mainJsonObj["cmpSet"].isArray()) {
        auto cmpArray = mainJsonObj["cmpSet"].toArray();
        for (const QJsonValue& cmpValue : cmpArray) {
            if (!cmpValue.isObject()) continue;
            QJsonObject cmpObj = cmpValue.toObject();
            QString objectType = cmpObj["objectType"].toString();

            if (objectType == "model" || objectType == "subSystem") {
                VarExpressionParse varParser;
                if (!mainVars.isEmpty()) {
                    varParser.setVariables(mainVars);
                }

                if (!parseSingleModel(mainLinkKey, cmpObj, appPath, outPutPath,
                                     mainSimuPara, mainVars, varParser,
                                     scopeMgr, resolver, mainBlocksInfo)) {
                    result.errorMessage = "解析模型和参数失败";
                    return result;
                }
            }
        }
    }

    if (!mainBlocksInfo.isEmpty()) {
        allBlocksInfo[mainLinkKey] = mainBlocksInfo;
    }

    // 解析主链路的连接关系
    if (!parseConnections(mainJsonObj, mainLinkKey, allConnections)) {
        result.errorMessage = "解析连接关系失败";
        return result;
    }

    // ===== 第五阶段：校验 =====
    // if (!dataCollectionCheck(allBlocksInfo)) {
    //     result.errorMessage = "数据收集器校验失败";
    //     return result;
    // }

    // ===== 存储结果 =====
    m_blocksInfo = allBlocksInfo;
    m_connections = allConnections;
    m_simuParameters = simuParams;

    result.success = true;
    result.mainLinkKey = mainLinkKey;
    result.simuParameters = simuParams;

    return result;
}

// ========== 获取解析结果 ==========
QMap<QString, QVector<BlockInfo>> LinkParser::getBlocksInfo() const
{
    return m_blocksInfo;
}

QMap<QString, QVector<Connection>> LinkParser::getConnections() const
{
    return m_connections;
}

QMap<QString, SimuParameter> LinkParser::getSimuParameters() const
{
    return m_simuParameters;
}
