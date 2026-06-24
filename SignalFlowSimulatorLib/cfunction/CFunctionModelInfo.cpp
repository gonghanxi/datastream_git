#include "CFunctionModelInfo.h"
#include "CFunctionBlock.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QJsonDocument>

QString CFunctionModelParser::extractId(const QString& id, const QString& prefix)
{
    if (id.startsWith(prefix)) {
        return id.mid(prefix.length());
    }
    return id;
}

bool CFunctionModelParser::parseConfigData(const QJsonObject& cmpObj, CFunctionModelInfo& modelInfo)
{
    if (!cmpObj.contains("configData") || !cmpObj["configData"].isObject()) {
        qDebug() << "[CFunction] No configData found for" << modelInfo.instanceName;
        return true;  // 非致命错误
    }

    QJsonObject configObj = cmpObj["configData"].toObject();

    // language
    modelInfo.configData.language = configObj["language"].toString();
    if (modelInfo.configData.language.isEmpty()) {
        modelInfo.configData.language = "cpp";
    }

    // libFiles
    if (configObj.contains("libFiles") && configObj["libFiles"].isArray()) {
        for (const QJsonValue& v : configObj["libFiles"].toArray()) {
            QJsonObject obj = v.toObject();
            CFunctionConfigData::FileEntry entry;
            entry.path = obj["path"].toString();
            entry.name = obj["name"].toString();
            modelInfo.configData.libFiles.append(entry);
        }
    }

    // headerFiles
    if (configObj.contains("headerFiles") && configObj["headerFiles"].isArray()) {
        for (const QJsonValue& v : configObj["headerFiles"].toArray()) {
            QJsonObject obj = v.toObject();
            CFunctionConfigData::FileEntry entry;
            entry.path = obj["path"].toString();
            entry.name = obj["name"].toString();
            modelInfo.configData.headerFiles.append(entry);
        }
    }

    // cFiles
    if (configObj.contains("cFiles") && configObj["cFiles"].isArray()) {
        for (const QJsonValue& v : configObj["cFiles"].toArray()) {
            QJsonObject obj = v.toObject();
            CFunctionConfigData::FileEntry entry;
            entry.path = obj["path"].toString();
            entry.name = obj["name"].toString();
            modelInfo.configData.cFiles.append(entry);
        }
    }

    qDebug() << "[CFunction] configData parsed:" << modelInfo.configData.language
             << "libs:" << modelInfo.configData.libFiles.size()
             << "headers:" << modelInfo.configData.headerFiles.size()
             << "sources:" << modelInfo.configData.cFiles.size();

    return true;
}

bool CFunctionModelParser::parseEquations(const QJsonObject& cmpObj, CFunctionModelInfo& modelInfo)
{
    if (cmpObj.contains("Equations") && cmpObj["Equations"].isString()) {
        modelInfo.equations = cmpObj["Equations"].toString();
        qDebug() << "[CFunction] Equations parsed, length:" << modelInfo.equations.length();
    }
    return true;
}

bool CFunctionModelParser::parseAttributes(
    const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QVector<Variable>& currentVars,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    CFunctionModelInfo& modelInfo)
{
    if (!cmpObj.contains("attribute") || !cmpObj["attribute"].isArray()) {
        return true;
    }

    QJsonArray attrArray = cmpObj["attribute"].toArray();
    for (const QJsonValue& attrVal : attrArray) {
        if (!attrVal.isObject()) continue;

        QJsonObject attrObj = attrVal.toObject();
        CFunctionParameter param;
        param.name = attrObj["name"].toString();
        param.dataType = attrObj["dataType"].toString();
        param.originalValue = attrObj["value"].toString();

        // 枚举类型解析 selectOptions
        if (param.dataType.toLower() == "enumeration" &&
            attrObj.contains("selectOptions") && attrObj["selectOptions"].isArray()) {
            for (const QJsonValue& opt : attrObj["selectOptions"].toArray()) {
                param.selectOptions.append(opt.toString());
            }
        }

        // 默认值处理
        if (param.originalValue.isEmpty() && attrObj.contains("valDefault")) {
            QJsonValue valDef = attrObj["valDefault"];
            if (valDef.isDouble()) param.originalValue = QString::number(valDef.toDouble());
            else if (valDef.isString()) param.originalValue = valDef.toString();
            else if (valDef.isBool()) param.originalValue = valDef.toBool() ? "true" : "false";
        }

        // 表达式解析
        QString calculateValueStr;
        if (attrObj.contains("calculateValue")) {
            QJsonValue cv = attrObj["calculateValue"];
            if (cv.isString()) calculateValueStr = cv.toString();
            else if (cv.isDouble()) calculateValueStr = QString::number(cv.toDouble());
        }

        param.value = resolveParameterValue(
            currentLinkKey, param.name, param.originalValue,
            calculateValueStr, currentVars, scopeMgr, resolver);

        // 枚举校验
        if (param.dataType.toLower() == "enumeration" && !param.selectOptions.isEmpty()) {
            if (!param.selectOptions.contains(param.value)) {
                LOG_ERROR("CFunction:", modelInfo.instanceName.toStdString(),
                          "param:", param.name.toStdString(),
                          "invalid enum value:", param.value.toStdString());
                return false;
            }
        }

        // 单位转换
        QString unitType = attrObj["unitType"].toString();
        if (unitType.isEmpty()) unitType = "NONE";
        QString unit = attrObj["unit"].toString();
        QString converted = UnitConvert::convertToStandardUnit(
            unitType, unit, param.dataType, param.value);
        if (!converted.contains("invalid_value")) {
            param.value = converted;
        }

        qDebug() << "[CFunction] attribute:" << param.name
                 << "value:" << param.value << "type:" << param.dataType;

        modelInfo.parameters[param.name] = param;
    }

    return true;
}

bool CFunctionModelParser::parsePorts(const QJsonObject& cmpObj, CFunctionModelInfo& modelInfo)
{
    if (!cmpObj.contains("port") || !cmpObj["port"].isArray()) {
        return true;
    }

    QJsonArray portArray = cmpObj["port"].toArray();
    for (const QJsonValue& portVal : portArray) {
        if (!portVal.isObject()) continue;

        QJsonObject portObj = portVal.toObject();
        CFunctionPortMsg port;
        port.id = extractId(portObj["id"].toString(), "p_").toInt();
        port.putType = portObj["putType"].toString();
        port.name = portObj["name"].toString();
        port.dataType = portObj["dataType"].toString();
        port.isOptional = portObj["isOptional"].toBool();

        QString portRate = portObj["PortRate"].toString();
        port.portRate = portRate.isEmpty() ? 1 : portRate.toInt();

        qDebug() << "[CFunction] port:" << port.name
                 << "dir:" << port.putType << "type:" << port.dataType;

        modelInfo.ports[port.id] = port;
    }

    return true;
}

QString CFunctionModelParser::resolveParameterValue(
    const QString& currentLinkKey,
    const QString& paramName,
    const QString& paramVal,
    const QString& calculateValueStr,
    const QVector<Variable>& currentVars,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver)
{
    QString finalValue = paramVal;

    ResolutionResult resolveResult = resolver.resolveExpression(
        currentLinkKey, paramVal, calculateValueStr);

    if (resolveResult.success) {
        return resolveResult.value;
    }

    if (!currentVars.isEmpty()) {
        VarExpressionParse varParser;
        varParser.setVariables(currentVars);

        if (varParser.isArrayExpression(paramVal)) {
            ExpressionResult parseResult = varParser.parseArrayWithExpressions(paramVal);
            if (parseResult.success) finalValue = parseResult.value;
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

bool CFunctionModelParser::parseCFunctionModel(
    const QString& currentLinkKey,
    const QJsonObject& cmpObj,
    const QVector<Variable>& currentVars,
    const SimuParameter& simuPara,
    VariableScopeManager& scopeMgr,
    ExpressionResolver& resolver,
    CFunctionModelInfo& outModelInfo)
{
    // 基础信息
    outModelInfo.cmpId = extractId(cmpObj["cmpId"].toString(), "cp_").toInt();
    outModelInfo.cmpType = cmpObj["cmpType"].toString();
    outModelInfo.instanceName = cmpObj["instanceName"].toString();
    outModelInfo.simuParams = simuPara;

    qDebug() << "========== CFunction parse begin ==========";
    qDebug() << "instance:" << outModelInfo.instanceName
             << "ID:" << outModelInfo.cmpId;

    if (!parseConfigData(cmpObj, outModelInfo)) return false;
    if (!parseEquations(cmpObj, outModelInfo)) return false;
    if (!parsePorts(cmpObj, outModelInfo)) return false;
    if (!parseAttributes(currentLinkKey, cmpObj, currentVars, scopeMgr, resolver, outModelInfo)) return false;

    qDebug() << "[CFunction] parse complete:" << outModelInfo.instanceName
             << "ports:" << outModelInfo.ports.size()
             << "params:" << outModelInfo.parameters.size();

    return true;
}

QString CFunctionModelParser::generateCFunctionJson(
    const CFunctionModelInfo& modelInfo,
    const QString& outputDir)
{
    QJsonObject root;

    // ===== configData =====
    QJsonObject configData;
    configData["language"] = modelInfo.configData.language;

    auto fileListToJsonArray = [](const QVector<CFunctionConfigData::FileEntry>& files) -> QJsonArray {
        QJsonArray arr;
        for (const auto& f : files) {
            QJsonObject obj;
            obj["path"] = f.path;
            obj["name"] = f.name;
            arr.append(obj);
        }
        return arr;
    };

    configData["libFiles"] = fileListToJsonArray(modelInfo.configData.libFiles);
    configData["headerFiles"] = fileListToJsonArray(modelInfo.configData.headerFiles);
    configData["cFiles"] = fileListToJsonArray(modelInfo.configData.cFiles);
    root["configData"] = configData;

    // ===== Equations =====
    root["Equations"] = modelInfo.equations;

    // ===== attribute =====
    QJsonArray attrArray;
    for (auto it = modelInfo.parameters.begin(); it != modelInfo.parameters.end(); ++it) {
        const CFunctionParameter& param = it.value();
        QJsonObject attrObj;
        attrObj["name"] = param.name;
        attrObj["datatype"] = param.dataType;
        attrObj["value"] = param.value;
        if (!param.selectOptions.isEmpty()) {
            QJsonArray opts;
            for (const QString& opt : param.selectOptions) opts.append(opt);
            attrObj["selectOptions"] = opts;
        }
        attrArray.append(attrObj);
    }
    root["attribute"] = attrArray;

    // ===== input =====
    QJsonArray inputArray;
    for (auto it = modelInfo.ports.begin(); it != modelInfo.ports.end(); ++it) {
        if (it.value().putType != "in") continue;
        const CFunctionPortMsg& port = it.value();
        QJsonObject portObj;
        portObj["name"] = port.name;
        portObj["datatype"] = port.dataType;
        // 根据数据类型设置默认值，避免首次运行时JSON值为空导致C++变量未定义
        QString defaultVal;
        QString dt = port.dataType.toLower();
        if (dt == "bool") defaultVal = "false";
        else if (dt == "complex") defaultVal = "[0,0]";
        else defaultVal = "0";  // real, int, matrix等数值类型
        portObj["value"] = defaultVal;
        inputArray.append(portObj);
    }
    root["input"] = inputArray;

    // ===== output =====
    QJsonArray outputArray;
    for (auto it = modelInfo.ports.begin(); it != modelInfo.ports.end(); ++it) {
        if (it.value().putType != "out") continue;
        const CFunctionPortMsg& port = it.value();
        QJsonObject portObj;
        portObj["name"] = port.name;
        portObj["datatype"] = port.dataType;
        portObj["value"] = "";  // 运行时由小引擎填充
        outputArray.append(portObj);
    }
    root["output"] = outputArray;

    // 写文件
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString fileName = QString("%1.json").arg(modelInfo.instanceName);
    QString filePath = dir.absoluteFilePath(fileName);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("[CFunction] Failed to write:", filePath.toStdString());
        return QString();
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[CFunction] Generated JSON:" << filePath;
    return filePath;
}

bool CFunctionModelParser::readCFunctionOutput(
    const QString& jsonPath,
    QVector<QPair<QString, QVector<double>>>& outputs)
{
    outputs.clear();

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("[CFunction] Failed to read output:", jsonPath.toStdString());
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        LOG_ERROR("[CFunction] Invalid JSON format:", jsonPath.toStdString());
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("output") || !root["output"].isArray()) {
        LOG_ERROR("[CFunction] Missing output array:", jsonPath.toStdString());
        return false;
    }

    QJsonArray outputArray = root["output"].toArray();
    for (const QJsonValue& outVal : outputArray) {
        if (!outVal.isObject()) continue;

        QJsonObject outObj = outVal.toObject();
        QString name = outObj["name"].toString();
        QString valueStr = outObj["value"].toString();

        QVector<double> values;
        if (!valueStr.isEmpty()) {
            // 支持单值和矩阵 "[1.0,2.0;3.0,4.0]"
            QString cleaned = valueStr;
            cleaned.remove('[').remove(']');
            QStringList rows = cleaned.split(';', QString::SkipEmptyParts);
            for (const QString& row : rows) {
                QStringList cols = row.split(',', QString::SkipEmptyParts);
                for (const QString& col : cols) {
                    bool ok = false;
                    double v = col.trimmed().toDouble(&ok);
                    if (ok) values.append(v);
                }
            }
        }

        outputs.append(qMakePair(name, values));
        qDebug() << "[CFunction] output:" << name << "values:" << values.size();
    }

    return true;
}
