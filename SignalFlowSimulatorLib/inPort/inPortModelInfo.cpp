#include "inPortModelInfo.h"
#include "inPortBlock.h"
#include <QDebug>

QString inPortModelParser::extractId(const QString& id, const QString& prefix)
{
    if (id.startsWith(prefix)) {
        return id.mid(prefix.length());
    }
    return id;
}


bool inPortModelParser::parsePorts(const QJsonObject& cmpObj, inPortModelInfo& modelInfo)
{
    if (!cmpObj.contains("port") || !cmpObj["port"].isArray()) {
        return true;
    }
    
    qDebug() << "========== inPort parsePorts begin ==========";
    auto portArray = cmpObj["port"].toArray();
    
    for (const QJsonValue& portValue : portArray) {
        if (!portValue.isObject()) continue;
        
        auto portObj = portValue.toObject();
        
        inPortPortMsg port;
        port.id = extractId(portObj["id"].toString(), "p_").toInt();
        port.putType = portObj["putType"].toString();
        port.name = portObj["name"].toString();
        port.dataType = portObj["dataType"].toString();
        port.isOptional = portObj["isOptional"].toBool();
        
        // 端口速率
        QString portRate = portObj["PortRate"].toString();
        port.portRate = portRate.toInt();
        
        // 顶层端口ID（用于子系统穿透）
        if (portObj.contains("topProtId")) {
            port.topProtId = extractId(portObj["topProtId"].toString(), "p_").toInt();
        }
        
        qDebug() << QString("inPort端口 ID:%1, 名称:%2, 方向:%3, 数据类型:%4")
                    .arg(port.id).arg(port.name).arg(port.putType).arg(port.dataType);
        
        modelInfo.inPorts[port.id] = port;
    }
    
    qDebug() << "inPort端口解析完成，端口数:" << modelInfo.inPorts.size();
    return true;
}

QString inPortModelParser::resolveParameterValue(
    const QString& currentLinkKey,
    const QString& paramName,
    const QString& paramVal,
    const QString& calculateValueStr,
    const QVector<Variable>& currentVars,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver)
{
    QString finalValue = paramVal;
    qDebug() << "inPortModelParser::resolveParameterValue - finalValue: " << finalValue;
    
    // 使用ExpressionResolver解析
    ResolutionResult resolveResult = resolver.resolveExpression(
        currentLinkKey, paramVal, calculateValueStr);
    
    if (resolveResult.success) {
        finalValue = resolveResult.value;
        qDebug() << "inPort参数解析成功:" << paramName
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

bool inPortModelParser::parseParameters(
    const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QVector<Variable>& currentVars,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    inPortModelInfo& modelInfo)
{
    if (!cmpObj.contains("attribute") || !cmpObj["attribute"].isArray()) {
        return true;
    }
    
    qDebug() << "========== inPort parseParameters begin ==========";
    auto paramArray = cmpObj["attribute"].toArray();
    
    for (const QJsonValue& paramValue : paramArray) {
        if (!paramValue.isObject()) continue;
        
        auto paramObj = paramValue.toObject();
        
        inPortParameter param;
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

        qDebug() << "inPortModelParser::parseParameters - 获取参数: ";
        qDebug() << "name: " << param.name;
        qDebug() << "dataType: " << param.dataType;
        qDebug() << "unitType: " << param.unitType;
        qDebug() << "unit: " << param.unit;
        qDebug() << "originalValue: " << param.originalValue;
        
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
                             "，inPort实例：", modelInfo.instanceName.toStdString(),
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
                     "，inPort实例：", modelInfo.instanceName.toStdString(),
                     "，参数：", param.name.toStdString(), "单位转换错误.");
            return false;
        }
        
        param.value = convertedValue;
        
        qDebug() << QString("inPort参数:%1, 原始值:%2, 转换值:%3")
                    .arg(param.name).arg(param.originalValue)
                    .arg(param.value);
        
        modelInfo.inPortParameters[param.name] = param;
    }
    
    qDebug() << "inPort参数解析完成，参数数:" << modelInfo.inPortParameters.size();
    return true;
}

bool inPortModelParser::parseinPortModel(const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QVector<Variable>& currentVars,
    const SimuParameter &simuPara,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    inPortModelInfo& outModelInfo)
{
    // 基础信息
    outModelInfo.cmpId = extractId(cmpObj["cmpId"].toString(), "cp_").toInt();
    outModelInfo.isSubSystem = cmpObj["isSubSystem"].toBool();
    outModelInfo.cmpType = cmpObj["cmpType"].toString();
    outModelInfo.instanceName = cmpObj["instanceName"].toString();
    outModelInfo.childTopoId = cmpObj["childTopoId"].toString();
    qDebug() << "inPortModelInfo::parseFMUModel - simuPara: " << simuPara.num_Samples;
    outModelInfo.m_Sima = simuPara;
    
    qDebug() << "========== parseinPortModel begin ==========";
    qDebug() << QString("inPort实例:%1, ID:%2")
                .arg(outModelInfo.instanceName)
                .arg(outModelInfo.cmpId);
    
    // 解析端口
    if (!parsePorts(cmpObj, outModelInfo)) {
        return false;
    }
    
    // 解析参数
    if (!parseParameters(currentLinkKey, cmpObj, currentVars, 
                         scopeMgr, resolver, outModelInfo)) {
        return false;
    }
    
    qDebug() << "inPort模型解析完成:" << outModelInfo.instanceName;
    
    return true;
}

Block *inPortModelInfo::createBlock() const
{
    using namespace SystemVueModelBuilder;

    inPortBlock* block = new inPortBlock(instanceName.toStdString());
    qDebug() << "inPortModelInfo::createBlock - block name: " << QString::fromStdString(block->GetName());

    // 设置基础信息
    qDebug() << "inPortModelInfo::createBlock - m_Sima: " << m_Sima.num_Samples;
    block->setSimuParams(m_Sima);

    // 添加端口信息
    for (auto it = inPorts.begin(); it != inPorts.end(); ++it) {
        const inPortPortMsg& inPort = it.value();

        // 转换为通用PortMsg
        PortMsg port;
        port.id = inPort.id;
        port.putType = inPort.putType;
        port.name = inPort.name;
        port.isOptional = inPort.isOptional;
        port.dataType = UnitConvert::convertToDataType(inPort.dataType);
        port.portRate = inPort.portRate;
        port.topProtId = inPort.topProtId;

        block->addPortInfo(port);
    }
    // 添加参数信息
    for (auto it = inPortParameters.begin(); it != inPortParameters.end(); ++it) {
        const inPortParameter& inPortParameter = it.value();
        QString paramName = inPortParameter.name;
        qDebug() << "inPortModelInfo::createBlock - parameter name: " << paramName;
        QString value = inPortParameter.value;
        qDebug() << "inPortModelInfo::createBlock - parameter value: " << value;
        block->addParameterInfo(inPortParameter.name,inPortParameter.value);
    }
    return block;
}


