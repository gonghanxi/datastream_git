#include "FMUModelInfo.h"
#include "FMUBlock.h"
#include <QDebug>

QString FMUModelParser::extractId(const QString& id, const QString& prefix)
{
    if (id.startsWith(prefix)) {
        return id.mid(prefix.length());
    }
    return id;
}

bool FMUModelParser::parseDllOrSoPaths(const QJsonObject& cmpObj, FMUModelInfo& modelInfo)
{
    if (!cmpObj.contains("dllORso") || !cmpObj["dllORso"].isArray()) {
        LOG_WARN("FMU模型缺少 dllORso 字段:", modelInfo.instanceName.toStdString());
        return true;  // 不是致命错误，继续
    }
    
    QJsonArray dllArray = cmpObj["dllORso"].toArray();
    for (const QJsonValue& dllValue : dllArray) {
        if (dllValue.isString()) {
            QString path = dllValue.toString();
            modelInfo.dllOrSoPaths.append(path);
            qDebug() << "FMU模型添加库路径:" << path 
                     << "实例:" << modelInfo.instanceName;
        }
    }
    
    if (modelInfo.dllOrSoPaths.isEmpty()) {
        LOG_WARN("FMU模型 dllORso 数组为空:", modelInfo.instanceName.toStdString());
    }
    
    return true;
}

bool FMUModelParser::parsePorts(const QJsonObject& cmpObj, FMUModelInfo& modelInfo)
{
    if (!cmpObj.contains("port") || !cmpObj["port"].isArray()) {
        return true;
    }
    
    qDebug() << "========== FMU parsePorts begin ==========";
    auto portArray = cmpObj["port"].toArray();
    
    for (const QJsonValue& portValue : portArray) {
        if (!portValue.isObject()) continue;
        
        auto portObj = portValue.toObject();
        
        FMUPortMsg port;
        port.id = extractId(portObj["id"].toString(), "p_").toInt();
        port.putType = portObj["putType"].toString();
        port.name = portObj["name"].toString();
        port.dataType = portObj["dataType"].toString();
        port.isOptional = portObj["isOptional"].toBool();
        
        // FMU特有：valueReference
        QString valueRefStr = portObj["valueReference"].toString();
        port.valueReference = valueRefStr.toInt();
        
        // 端口速率
        QString portRate = portObj["PortRate"].toString();
        port.portRate = portRate.toInt();
        
        // 顶层端口ID（用于子系统穿透）
        if (portObj.contains("topProtId")) {
            port.topProtId = extractId(portObj["topProtId"].toString(), "p_").toInt();
        }
        
        qDebug() << QString("FMU端口 ID:%1, 名称:%2, 方向:%3, valueReference:%4, 数据类型:%5")
                    .arg(port.id).arg(port.name).arg(port.putType)
                    .arg(port.valueReference).arg(port.dataType);
        
        modelInfo.fmuPorts[port.id] = port;
    }
    
    qDebug() << "FMU端口解析完成，端口数:" << modelInfo.fmuPorts.size();
    return true;
}

QString FMUModelParser::resolveParameterValue(
    const QString& currentLinkKey,
    const QString& paramName,
    const QString& paramVal,
    const QString& calculateValueStr,
    const QVector<Variable>& currentVars,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver)
{
    QString finalValue = paramVal;
    qDebug() << "FMUModelParser::resolveParameterValue - finalValue: " << finalValue;
    
    // 使用ExpressionResolver解析
    ResolutionResult resolveResult = resolver.resolveExpression(
        currentLinkKey, paramVal, calculateValueStr);
    
    if (resolveResult.success) {
        finalValue = resolveResult.value;
        qDebug() << "FMU参数解析成功:" << paramName 
                 << paramVal << "->" << finalValue;
        return finalValue;
    }
    
    // Fallback to old parser
    if (!currentVars.isEmpty()) {
        VarExpressionParse varParser;
        varParser.setVariables(currentVars);
        
        if (varParser.isArrayExpression(paramVal)) {
            ExpressionResult parseResult = varParser.parseArrayWithExpressions(paramVal);
            if (parseResult.success) {
                finalValue = parseResult.value;
            }
        } else {
            ExpressionResult parseResult = varParser.parseExpression(paramVal);
            if (parseResult.success) {
                finalValue = parseResult.value;
            } else if (!calculateValueStr.isEmpty()) {
                finalValue = calculateValueStr;
            }
        }
    } else if (!calculateValueStr.isEmpty()) {
        finalValue = calculateValueStr;
    }
    
    return finalValue;
}

bool FMUModelParser::parseParameters(
    const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QVector<Variable>& currentVars,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    FMUModelInfo& modelInfo)
{
    if (!cmpObj.contains("attribute") || !cmpObj["attribute"].isArray()) {
        return true;
    }
    
    qDebug() << "========== FMU parseParameters begin ==========";
    auto paramArray = cmpObj["attribute"].toArray();
    
    for (const QJsonValue& paramValue : paramArray) {
        if (!paramValue.isObject()) continue;
        
        auto paramObj = paramValue.toObject();
        
        FMUParameter param;
        param.name = paramObj["name"].toString();
        param.dataType = paramObj["dataType"].toString();
        param.unitType = paramObj["unitType"].toString();
        param.unit = paramObj["unit"].toString();
        param.originalValue = paramObj["value"].toString();
        //如果单位类型没有，则先默认为NONE
        if(param.unitType.isEmpty()) {
            param.unitType = "NONE";
        }
        //如果值没有，使用valDefault
        if(param.originalValue.isEmpty()) {
            if(paramObj.contains("valDefault")) {
                QJsonValue valDefault = paramObj["valDefault"];
                if (valDefault.isDouble()) {
                    param.originalValue = QString::number(valDefault.toDouble());
                } else if (valDefault.isString()) {
                    param.originalValue = valDefault.toString();
                } else if (valDefault.isBool()) {
                    param.originalValue = valDefault.toBool() ? "true" : "false";
                }
            }
        }

        qDebug() << "FMUModelParser::parseParameters - 获取参数: ";
        qDebug() << "name: " << param.name;
        qDebug() << "dataType: " << param.dataType;
        qDebug() << "unitType: " << param.unitType;
        qDebug() << "unit: " << param.unit;
        qDebug() << "originalValue: " << param.originalValue;
        
        // FMU特有：valueReference
        QString valueRefStr = paramObj["valueReference"].toString();
        param.valueReference = valueRefStr.toInt();
        
        // 默认值处理
        param.isDefault = paramObj["isDefault"].toBool(false);
        if (param.isDefault) {
            if (paramObj.contains("valDefault")) {
                QJsonValue valDefault = paramObj["valDefault"];
                if (valDefault.isDouble()) {
                    param.originalValue = QString::number(valDefault.toDouble());
                } else if (valDefault.isString()) {
                    param.originalValue = valDefault.toString();
                } else if (valDefault.isBool()) {
                    param.originalValue = valDefault.toBool() ? "true" : "false";
                }
            } else if (paramObj.contains("defaultValue")) {
                QJsonValue defaultValue = paramObj["defaultValue"];
                if (defaultValue.isDouble()) {
                    param.originalValue = QString::number(defaultValue.toDouble());
                } else if (defaultValue.isString()) {
                    param.originalValue = defaultValue.toString();
                } else if (defaultValue.isBool()) {
                    param.originalValue = defaultValue.toBool() ? "true" : "false";
                }
            }
        }
        
        // 获取calculateValue
        if (paramObj.contains("calculateValue")) {
            QJsonValue calculateValue = paramObj["calculateValue"];
            if (calculateValue.isDouble()) {
                param.calculateValue = QString::number(calculateValue.toDouble());
            } else if (calculateValue.isString()) {
                param.calculateValue = calculateValue.toString();
            } else if (calculateValue.isBool()) {
                param.calculateValue = calculateValue.toBool() ? "true" : "false";
            }
        }
        
        // 解析表达式
        param.value = resolveParameterValue(
            currentLinkKey, param.name, param.originalValue, 
            param.calculateValue, currentVars, scopeMgr, resolver);
        
        // 参数校验
        if (param.dataType.toLower() == "enumeration") {
            if (paramObj.contains("selectOptions") && paramObj["selectOptions"].isArray()) {
                QJsonArray selectOptions = paramObj["selectOptions"].toArray();
                QString enumError = UnitConvert::validateEnumeration(param.value, selectOptions);
                if (!enumError.isEmpty()) {
                    LOG_ERROR("链路：", currentLinkKey.toStdString(),
                             "，FMU实例：", modelInfo.instanceName.toStdString(),
                             "，参数：", param.name.toStdString(), " - ", enumError.toStdString());
                    return false;
                }
            }
        }
        
        // 单位转换
        QString convertedValue = UnitConvert::convertToStandardUnit(
            param.unitType, param.unit, param.dataType, param.value);
        
        if (convertedValue.contains("invalid_value")) {
            LOG_ERROR("链路：", currentLinkKey.toStdString(),
                     "，FMU实例：", modelInfo.instanceName.toStdString(),
                     "，参数：", param.name.toStdString(), "单位转换错误.");
            return false;
        }
        
        param.value = convertedValue;
        
        qDebug() << QString("FMU参数:%1, 原始值:%2, 转换值:%3, valueReference:%4")
                    .arg(param.name).arg(param.originalValue)
                    .arg(param.value).arg(param.valueReference);
        
        modelInfo.fmuParameters[param.name] = param;
    }
    
    qDebug() << "FMU参数解析完成，参数数:" << modelInfo.fmuParameters.size();
    return true;
}

bool FMUModelParser::parseFMUModel(const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QVector<Variable>& currentVars,
    const SimuParameter &simuPara,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    FMUModelInfo& outModelInfo)
{
    // 基础信息
    outModelInfo.cmpId = extractId(cmpObj["cmpId"].toString(), "cp_").toInt();
    outModelInfo.isSubSystem = cmpObj["isSubSystem"].toBool();
    outModelInfo.cmpType = cmpObj["cmpType"].toString();
    outModelInfo.instanceName = cmpObj["instanceName"].toString();
    outModelInfo.childTopoId = cmpObj["childTopoId"].toString();
    qDebug() << "FMUModelInfo::parseFMUModel - simuPara: " << simuPara.num_Samples;
    outModelInfo.m_Sima = simuPara;

    // 解析GUID字段（与cmpType同级）
    if (cmpObj.contains("guid") && cmpObj["guid"].isString()) {
        outModelInfo.guid = cmpObj["guid"].toString();
        qDebug() << "FMU模型GUID:" << outModelInfo.guid;
    } else {
        LOG_WARN("FMU模型缺少guid字段:", outModelInfo.instanceName.toStdString());
    }
    
    qDebug() << "========== parseFMUModel begin ==========";
    qDebug() << QString("FMU实例:%1, ID:%2, GUID:%3")
                .arg(outModelInfo.instanceName)
                .arg(outModelInfo.cmpId)
                .arg(outModelInfo.guid);
    
    // 解析 dllORso 路径
    if (!parseDllOrSoPaths(cmpObj, outModelInfo)) {
        return false;
    }
    
    // 解析端口
    if (!parsePorts(cmpObj, outModelInfo)) {
        return false;
    }
    
    // 解析参数
    if (!parseParameters(currentLinkKey, cmpObj, currentVars, 
                         scopeMgr, resolver, outModelInfo)) {
        return false;
    }
    
    qDebug() << "FMU模型解析完成:" << outModelInfo.instanceName
             << "GUID:" << outModelInfo.guid
             << "库路径数:" << outModelInfo.dllOrSoPaths.size()
             << "端口数:" << outModelInfo.fmuPorts.size()
             << "参数数:" << outModelInfo.fmuParameters.size();
    
    return true;
}



Block *FMUModelInfo::createBlock() const
//FMUBlock *FMUModelInfo::createBlock() const
{
    using namespace SystemVueModelBuilder;

//    Block* block = new Block();
    FMUBlock* block = new FMUBlock(instanceName.toStdString());
    qDebug() << "FMUModelInfo::createBlock - block name: " << QString::fromStdString(block->GetName());

    // 设置基础信息
    block->setFMUConfig(guid, instanceName, cmpId);
    block->setDllPaths(dllOrSoPaths);
    qDebug() << "FMUModelInfo::createBlock - m_Sima: " << m_Sima.num_Samples;
    block->setSimuParams(m_Sima);

    // 添加端口信息
    for (auto it = fmuPorts.begin(); it != fmuPorts.end(); ++it) {
        const FMUPortMsg& fmuPort = it.value();

        // 转换为通用PortMsg
        PortMsg port;
        port.id = fmuPort.id;
        port.putType = fmuPort.putType;
        port.name = fmuPort.name;
        port.isOptional = fmuPort.isOptional;
        port.dataType = UnitConvert::convertToDataType(fmuPort.dataType);
        port.portRate = fmuPort.portRate;
        port.topProtId = fmuPort.topProtId;

        block->addPortInfo(port, fmuPort.valueReference);
    }
    // 添加参数信息
    for (auto it = fmuParameters.begin(); it != fmuParameters.end(); ++it) {
        const FMUParameter& fmuParameter = it.value();
        QString paramName = fmuParameter.name;
        qDebug() << "FMUModelInfo::createBlock - parameter name: " << paramName;
        int valueReference = fmuParameter.valueReference;
        qDebug() << "FMUModelInfo::createBlock - parameter reference: " << valueReference;
        QString value = fmuParameter.value;
        qDebug() << "FMUModelInfo::createBlock - parameter value: " << value;
        block->addParameterInfo(fmuParameter.name, fmuParameter.valueReference, fmuParameter.value);
    }
    return block;
}


