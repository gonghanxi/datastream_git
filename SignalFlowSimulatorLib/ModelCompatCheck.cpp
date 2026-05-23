#include "ModelCompatCheck.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

ModelCompatCheck::ModelCompatCheck()
{
    initializeWhitelist();
    initializeComponentTypeWhitelist();
    m_reportedUnknownFields.clear();
    m_reportedComponentTypes.clear();
}

ModelCompatCheck::~ModelCompatCheck()
{
    // 白名单（所有支持的字段）
    m_whitelist.clear();

    // 组件类型白名单
    m_componentTypeWhitelist.clear();

    // 需要忽略的字段（如前端专用字段）
    m_ignoredFields.clear();

    // 连接对象忽略字段
    m_connectIgnoreFields.clear();

    // 组件对象忽略字段
    m_cmpIgnoreFields.clear();

    // 不同对象类型的必填字段
    m_requiredFields.clear();

    // 用于避免重复警告
    m_reportedUnknownFields.clear();
    m_reportedComponentTypes.clear();
}

CompatibilityResult ModelCompatCheck::checkCompatibility(const QJsonObject &jsonObj, const QString &context)
{
    CompatibilityResult result;
    QString currentContext = context.isEmpty() ? "root" : context;

    // 检查所有字段
    checkObjectFields(jsonObj, currentContext, result);

    // 检查特定对象类型的必填字段
    QString objType = getObjectType(jsonObj);
    checkRequiredFields(jsonObj, objType, currentContext, result);

    // 特殊检查：组件类型有效性
    if (jsonObj.contains(CMP_TYPE) && jsonObj[CMP_TYPE].isString()) {
        QString cmpType = jsonObj[CMP_TYPE].toString();
        checkComponentType(cmpType, currentContext, result);
    }

    return result;
}

CompatibilityResult ModelCompatCheck::checkLinkFile(const QJsonDocument &jsonDoc, const QString &filePath)
{
    CompatibilityResult result;
    QString context = filePath.isEmpty() ? "链路文件" : QString("文件: %1").arg(filePath);

    if (jsonDoc.isNull()) {
        result.addError(QString("%1: JSON文档为空").arg(context));
        return result;
    }

    // 清空前一次检查的记录
    m_reportedUnknownFields.clear();
    m_reportedComponentTypes.clear();

    if (jsonDoc.isArray()) {
        QJsonArray array = jsonDoc.array();
        for (int i = 0; i < array.size(); ++i) {
            if (array[i].isObject()) {
                QJsonObject obj = array[i].toObject();
                QString objContext = QString("%1[%2]").arg(context).arg(i);

                // 检查顶层对象字段
                checkObjectFields(obj, objContext, result);

                // 检查顶层对象必填字段
                checkRequiredFields(obj, "project_top", objContext, result);

                // 特殊处理：检查cmpSet数组
                if (obj.contains(PROJECT_CMP_SET) && obj[PROJECT_CMP_SET].isArray()) {
                    checkArrayFields(obj[PROJECT_CMP_SET].toArray(),
                                    objContext + "." + PROJECT_CMP_SET, result);
                }

                // 特殊处理：检查ConnectSet数组
                if (obj.contains(CONNECT_SET) && obj[CONNECT_SET].isArray()) {
                    checkArrayFields(obj[CONNECT_SET].toArray(),
                                    objContext + "." + CONNECT_SET, result);
                }

                // 检查simuParams对象
                if (obj.contains("simuParams") && obj["simuParams"].isObject()) {
                    QJsonObject simuParams = obj["simuParams"].toObject();
                    checkObjectFields(simuParams, objContext + ".simuParams", result);
                    checkRequiredFields(simuParams, "simu_params", objContext + ".simuParams", result);
                }
            }
        }
    } else if (jsonDoc.isObject()) {
        checkObjectFields(jsonDoc.object(), context, result);
        checkRequiredFields(jsonDoc.object(), "project_top", context, result);
    }

    return result;
}

void ModelCompatCheck::initializeWhitelist()
{
    // ============================================================================
    // 项目顶层字段
    // ============================================================================
    m_whitelist = {
        // 项目基本信息
        PROJECT_MAX_CMP_SET_ID,
        PROJECT_LINK_KEY,
        PROJECT_INST_NAME_TABLE,
        PROJECT_OBJECT_TYPE,
        PROJECT_CMP_AUTO_ID,
        PROJECT_LINK_KEY_TABLE,
        PROJECT_LINK_NET_NAME_TABLE,
        PROJECT_LINK_DELETE_LIST,
        PROJECT_CMP_DELETE_LIST,
        PROJECT_TOPO_ID,
        PROJECT_NAME,
        PROJECT_MAX_CT_ID,
        PROJECT_MAX_PROT_ID,
        PROJECT_IS_INTERFACE,
        PROJECT_CMP_SET,
        PROJECT_LINK_AUTO_ID,

        // 连接集合字段
        CONNECT_SET,

        // 连接对象字段
        CONN_CMP_ID_START,
        CONN_PORT_ID_END,
        CONN_NET_NAME,
        CONN_CMP_ID_END,
        CONN_ID,
        CONN_PORT_ID_START,

        // 组件对象字段
        "cmpCondition",
        CMP_INSTANCE_NAME,
        CMP_ADS_LIB,
        CMP_CATEGORY,
        CMP_ID,
        CMP_ICON,
        CMP_IS_INSTANCE_NAME_SHOW,
        CMP_SCALE,
        CMP_IS_SUB_SYSTEM,
        CMP_OBJECT_TYPE,
        CMP_LABEL_POSITION_STATE,
        CMP_ICON_OBJECT,
        CMP_SIZE,
        CMP_PORT,
        CMP_POS,
        CMP_IS_CMP_TYPE_SHOW,
        CMP_ICON_WEB,
        CMP_TYPE,
        CMP_ATTRIBUTE,
        CMP_IS_PORTEDIT,
        CMP_PORT_ID_MAP,

        // 子系统相关字段（新增）
        "childModelName",
        "childTopoId",
        "childJson",

        // 公共变量相关字段（新增）
        "vars",

        // 端口对象字段
        PORT_ROTATE,
        PORT_PUT_TYPE,
        PORT_POS,
        PORT_NAME_DISPLAY,
        PORT_DATA_TYPE,
        PORT_CMP_ID,
        PORT_NAME,
        PORT_NUM_IN_CMP,
        PORT_ID,
        PORT_LABEL,
        PORT_LINE_LENGTH,
        "topProtId",
        "isOptional",

        // 属性对象字段
        ATTR_UNIT_TYPE,
        ATTR_UNIT,
        ATTR_VAL_DEFAULT,
        ATTR_DATA_TYPE,
        ATTR_NAME,
        ATTR_HIDE_CONDITION,
        ATTR_IS_DISP,
        ATTR_VALUE,
        ATTR_DESC,
        "selectOptions",
        "calculateValue",

        // 项目相关字段
        "simuParams",
        "projectId",
        "description",
        "Equations",

        // 仿真参数子字段
        "Num_Samples",
        "simuName",
        "SamplingRate_Unit",
        "Time_Interval",
        "Time_Interval_Unit",
        "StartTime",
        "describe",
        "StopTime_Unit",
        "StartTime_Unit",
        "SamplingRate",
        "StopTime",

        // 变量相关字段（vars数组中的字段）
        "enums",
        "constraint",
        "defaultValue",
        "disp",

        // 特殊属性字段
        "StartStopOption",
        "SampleStart",
        "SampleStop",
        "TimeStart",
        "TimeStop",
        "FileName",
        "PORT",
        "Data Type",
        "Bus"
    };

    // 设置忽略的字段（前端专用，不影响引擎运行）
    m_ignoredFields = {
        CMP_ICON,
        CMP_ICON_OBJECT,
        CMP_ICON_WEB,
        CMP_LABEL_POSITION_STATE,
        CMP_IS_INSTANCE_NAME_SHOW,
        CMP_IS_CMP_TYPE_SHOW,
        CMP_SCALE,
        CMP_POS,
        CMP_SIZE,
        PROJECT_INST_NAME_TABLE,
        PROJECT_LINK_KEY_TABLE,
        PROJECT_LINK_NET_NAME_TABLE,
        PROJECT_LINK_DELETE_LIST,
        PROJECT_CMP_DELETE_LIST,
        PROJECT_CMP_AUTO_ID,
        PROJECT_LINK_AUTO_ID,
        "describe",
        "icon",  // 兼容旧字段
        "iconObject",
        "iconWeb",
        "labelPositionState",
        "isInstanceNameShow",
        "isCmpTypeShow",
        "scale",
        "pos",
        "size"
    };

    // 连接对象忽略字段（前端专用，不影响连接逻辑）
    m_connectIgnoreFields = {
        "netSymbol",
        "newId",
        "oldId",
        "style",
        "type",
        "port",
        "name",
        "cmpId",  // ConnectSet中的cmpId与cmpIdStart/cmpIdEnd重复
        "portIdMap"
    };

    // 组件对象忽略字段（前端专用，不影响模型逻辑）
    m_cmpIgnoreFields = {
        "newId",
        "oldId",
        "type",
        "Equations",
        "description"
    };

    // ============================================================================
    // 必填字段定义
    // ============================================================================

    // 项目顶层对象必填字段
    m_requiredFields["project_top"] = {
        PROJECT_LINK_KEY,
        PROJECT_NAME,
        PROJECT_CMP_SET
    };

    // 组件对象必填字段
    m_requiredFields["cmp_item"] = {
        CMP_ID,
        CMP_TYPE,
        CMP_INSTANCE_NAME,
        "cmpCondition"  // 注意：这个字段是必要的
    };

    // 端口对象必填字段
    m_requiredFields["port_item"] = {
        PORT_ID,
        PORT_NAME,
        PORT_PUT_TYPE,
        PORT_DATA_TYPE
    };

    // 属性对象必填字段
    m_requiredFields["attribute_item"] = {
        ATTR_NAME,
        ATTR_DATA_TYPE,
        ATTR_VALUE
    };

    // 连接对象必填字段
    m_requiredFields["connection_item"] = {
        CONN_ID,
        CONN_CMP_ID_START,
        CONN_PORT_ID_START,
        CONN_CMP_ID_END,
        CONN_PORT_ID_END
    };

    // 仿真参数对象必填字段
    m_requiredFields["simu_params"] = {
        "Num_Samples",
        "SamplingRate",
        "StartTime",
        "StopTime"
    };

    // 添加变量对象的必填字段
    m_requiredFields["var_item"] = {
        VAR_NAME,
        VAR_DATA_TYPE,
        VAR_ID
    };
}

void ModelCompatCheck::initializeComponentTypeWhitelist()
{
    // 从JsonLinkDefine.h中获取组件类型
    m_componentTypeWhitelist = {
        CMP_TYPE_GROUND,
        CMP_TYPE_VAC,
        CMP_TYPE_VDC,
        CMP_TYPE_V_1TONE,
        CMP_TYPE_V_NTONE,
        CMP_TYPE_Term,
        CMP_TYPE_R,
        CMP_TYPE_C,
        CMP_TYPE_DIODE,
        CMP_TYPE_Amplifier2,
        CMP_TYPE_Amplifier,
        CMP_TYPE_bjt,
        CMP_TYPE_Attenuator,
        CMP_TYPE_BSF_Butterworth,
        CMP_TYPE_HPF_Butterworth,
        CMP_TYPE_HPF_Chebyshev,
        CMP_TYPE_inductor,
        CMP_TYPE_LPF_Butterworth,
        CMP_TYPE_LPF_Chebyshev,
        CMP_TYPE_PwrSplit2,
        CMP_TYPE_PwrSplit3,
        CMP_TYPE_TwoPort,

        CMP_TYPE_subSystem,
        CMP_TYPE_inPort,
        CMP_TYPE_outPort,

        CMP_TYPE_Const,
        CMP_TYPE_ConstCx,
        CMP_TYPE_GaussianNoiseGen,
        CMP_TYPE_RADAR_CW ,
        CMP_TYPE_RADAR_PULSE,

        CMP_TYPE_Abs_M,
        CMP_TYPE_AdaptLinQuant,
        CMP_TYPE_Add,
        CMP_TYPE_AddCx,
        CMP_TYPE_AddEnv,
        CMP_TYPE_AddEnv_M,
        CMP_TYPE_AddGuard,
        CMP_TYPE_AddInt,
        CMP_TYPE_AsyncCommutator,
        CMP_TYPE_AsyncCommutatorCx,
        CMP_TYPE_AsyncCommutatorEnv,
        CMP_TYPE_AsyncCommutatorInt,
        CMP_TYPE_AsyncDistributor,
        CMP_TYPE_AsyncDistributorCx,
        CMP_TYPE_AsyncDistributorEnv,
        CMP_TYPE_AsyncDistributorInt,
        CMP_TYPE_AutoCorr,
        CMP_TYPE_Average,
        CMP_TYPE_AverageCx,
        CMP_TYPE_AverageCxWOffset,
        CMP_TYPE_AvgSqrErr_M,

        CMP_TYPE_BCH_Decoder,
        CMP_TYPE_BCH_Encoder,
        CMP_TYPE_Biquad,
        CMP_TYPE_BiquadCascade,
        CMP_TYPE_BitDeformatter,
        CMP_TYPE_BitFormatter,
        CMP_TYPE_Bits,
        CMP_TYPE_BitShiftRegister,
        CMP_TYPE_BitsToInt,
        CMP_TYPE_BlockAllPole,
        CMP_TYPE_BPF_Butterworth,
        CMP_TYPE_BPF_ChebyshevI,
        CMP_TYPE_BPF_ChebyshevII,
        CMP_TYPE_BSF_Butterworth,
        CMP_TYPE_BSF_ChebyshevI,
        CMP_TYPE_BSF_ChebyshevII,

        CMP_TYPE_ChirpGen,
        CMP_TYPE_Chop,
        CMP_TYPE_ChopCx,
        CMP_TYPE_ChopInt,
        CMP_TYPE_ChopVarOffset,
        CMP_TYPE_ChopVarOffsetCx,
        CMP_TYPE_ChopVarOffsetInt,
        CMP_TYPE_CoderRs,

        CMP_TYPE_Gain,
        CMP_TYPE_Math,
        CMP_TYPE_RectToCx,
        CMP_TYPE_Sub,


        CMP_TYPE_Sink,
        CMP_TYPE_SinkCx,
        CMP_TYPE_SinkEnv
    };
}

void ModelCompatCheck::checkObjectFields(const QJsonObject &obj, const QString &parentPath, CompatibilityResult &result)
{
    QString currentPath = parentPath.isEmpty() ? "root" : parentPath;
    QString objType = getObjectType(obj);

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString fieldName = it.key();

        // 检查是否需要检查此字段
        if (!shouldCheckField(fieldName, objType)) {
            continue;
        }

        // 检查字段是否在白名单中
        if (!m_whitelist.contains(fieldName)) {
            QString fullFieldPath = currentPath + "." + fieldName;

            // 避免重复报告相同的未知字段
            if (!m_reportedUnknownFields.contains(fullFieldPath)) {
                m_reportedUnknownFields.insert(fullFieldPath);

                QString warning = QString("%1: 检测到未知字段 '%2'，可能是新版本字段")
                                 .arg(currentPath).arg(fieldName);

                if (m_strictMode) {
                    result.addError(warning);
                } else {
                    result.addWarning(warning);
                }
            }
        }

        // 递归检查嵌套对象
        if (it.value().isObject()) {
            checkObjectFields(it.value().toObject(),
                            currentPath + "." + fieldName, result);
        }

        // 递归检查数组中的对象
        if (it.value().isArray()) {
            checkArrayFields(it.value().toArray(),
                           currentPath + "." + fieldName, result);
        }
    }

    // 特殊检查：公共变量数组
    if (obj.contains(VARS) && obj[VARS].isArray()) {
        QJsonArray varsArray = obj[VARS].toArray();
        for (int i = 0; i < varsArray.size(); ++i) {
            if (varsArray[i].isObject()) {
                QJsonObject varObj = varsArray[i].toObject();
                QString varPath = QString("%1.%2[%3]").arg(parentPath).arg(VARS).arg(i);

                // 检查变量对象字段
                checkObjectFields(varObj, varPath, result);

                // 检查变量对象的必填字段
                checkRequiredFields(varObj, "var_item", varPath, result);
            }
        }
    }

    // 特殊检查：组件类型有效性
    if (objType == "cmp_item" && obj.contains(CMP_TYPE) && obj[CMP_TYPE].isString()) {
        QString cmpType = obj[CMP_TYPE].toString();
        QString componentPath = currentPath + "." + CMP_TYPE;

        // 避免重复报告相同的组件类型
        if (!m_reportedComponentTypes.contains(componentPath)) {
            m_reportedComponentTypes.insert(componentPath);
            checkComponentType(cmpType, currentPath, result);
        }
    }
}

void ModelCompatCheck::checkArrayFields(const QJsonArray &array, const QString &parentPath, CompatibilityResult &result)
{
    for (int i = 0; i < array.size(); ++i) {
        if (array[i].isObject()) {
            QJsonObject obj = array[i].toObject();
            QString elementPath = QString("%1[%2]").arg(parentPath).arg(i);

            // 检查对象字段
            checkObjectFields(obj, elementPath, result);

            // 确定数组元素的类型并检查必填字段
            QString objType = getObjectType(obj);
            checkRequiredFields(obj, objType, elementPath, result);

            // 特殊检查：端口和属性数组中的数据类型
            if (objType == "port_item" || objType == "attribute_item") {
                // 检查必填字段已经在上面处理了
            }
        }
    }
}

QString ModelCompatCheck::getObjectType(const QJsonObject &obj) const
{
    // 根据字段判断对象类型，按特定顺序检查

    // 1. 端口对象 - 优先检查，因为端口也有id和name
    if (obj.contains(PORT_ID) && obj.contains(PORT_PUT_TYPE)) {
        return "port_item";
    }

    // 2. 属性对象
    if (obj.contains(ATTR_NAME) && obj.contains(ATTR_DATA_TYPE)) {
        return "attribute_item";
    }

    // 3. 连接对象
    if ((obj.contains(CONN_CMP_ID_START) && obj.contains(CONN_PORT_ID_START))) {
        return "connection_item";
    }

    // 4. 组件对象
    if (obj.contains(CMP_ID) && obj.contains(CMP_TYPE)) {
        return "cmp_item";
    }

    // 5. 仿真参数对象
    if (obj.contains("simuParams") ||
        obj.contains("Num_Samples") ||
        obj.contains("SamplingRate")) {
        return "simu_params";
    }

    // 6. 项目顶层对象
    if (obj.contains(PROJECT_LINK_KEY) && obj.contains(PROJECT_CMP_SET)) {
        return "project_top";
    }

    // 7. 变量对象（放在端口和属性之后，因为它们可能有类似字段）
    if ((obj.contains(VAR_NAME) && obj.contains(VAR_DATA_TYPE) && obj.contains(VAR_ID)) ||
            (obj.contains("name") && obj.contains("dataType") && obj.contains("id") &&
             !obj.contains(PORT_PUT_TYPE) && !obj.contains(ATTR_HIDE_CONDITION))) {
        return "var_item";
    }

    return "unknown";
}

bool ModelCompatCheck::shouldCheckField(const QString &fieldName, const QString &objType) const
{
    // 忽略前端专用字段
    if (m_ignoredFields.contains(fieldName)) {
        return false;
    }

    // 根据对象类型应用特定的忽略规则
    if (objType == "connection_item") {
        if (m_connectIgnoreFields.contains(fieldName)) {
            return false;
        }
    } else if (objType == "cmp_item") {
        if (m_cmpIgnoreFields.contains(fieldName)) {
            return false;
        }
    }

    // 忽略某些内部字段
    if (fieldName.startsWith("_") || fieldName.startsWith("__")) {
        return false;
    }

    return true;
}

void ModelCompatCheck::checkRequiredFields(const QJsonObject &obj, const QString &objType, const QString &context, CompatibilityResult &result)
{
    if (!m_requiredFields.contains(objType)) {
        return;
    }

    const QSet<QString>& requiredFields = m_requiredFields[objType];
    for (const QString& field : requiredFields) {
        if (!obj.contains(field)) {
            QString error = QString("%1: 缺少必填字段 '%2'")
                           .arg(context).arg(field);
            result.addError(error);
        }
    }
}

void ModelCompatCheck::checkComponentType(const QString &cmpType, const QString &context, CompatibilityResult &result)
{
    if (!m_componentTypeWhitelist.contains(cmpType)) {
        QString warning = QString("%1: 未知的组件类型 '%2'，可能是新版本的组件")
                         .arg(context).arg(cmpType);
        result.addWarning(warning);
    }
}
