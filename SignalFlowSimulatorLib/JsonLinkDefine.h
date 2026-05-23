#ifndef JSONLINKDEFINE_H
#define JSONLINKDEFINE_H
// ============================================================================
// 项目顶层字段宏定义
// ============================================================================

// 项目基本信息
#define PROJECT_MAX_CMP_SET_ID       "maxCmpSetId"           // 最大组件集合ID
#define PROJECT_LINK_KEY             "linkkey"               // 链路键值
#define PROJECT_INST_NAME_TABLE      "instNameTable"         // 实例名称表
#define PROJECT_OBJECT_TYPE          "objectType"            // 对象类型
#define PROJECT_CMP_AUTO_ID          "cmpAutoId"             // 组件自动ID
#define PROJECT_LINK_KEY_TABLE       "linkKeyTable"          // 链路键值表
#define PROJECT_LINK_NET_NAME_TABLE  "linkNetNameTable"      // 链路网络名称表
#define PROJECT_LINK_DELETE_LIST     "linkDeleteList"        // 链路删除列表
#define PROJECT_CMP_DELETE_LIST      "cmpDeleteList"         // 组件删除列表
#define PROJECT_TOPO_ID              "topoId"                // 拓扑ID
#define PROJECT_NAME                 "name"                  // 项目名称
#define PROJECT_MAX_CT_ID            "maxCtId"               // 最大连接ID
#define PROJECT_MAX_PROT_ID          "maxProtId"             // 最大端口ID
#define PROJECT_IS_INTERFACE         "isInterface"           // 是否是接口
#define PROJECT_CMP_SET              "cmpSet"                // 组件集合
#define PROJECT_LINK_AUTO_ID         "linkAutoId"            // 链路自动ID

// 连接集合字段
#define CONNECT_SET                  "ConnectSet"            // 连接集合

// ============================================================================
// 连接对象字段宏定义 (ConnectSet)
// ============================================================================

// 单个连接对象字段
#define CONN_CMP_ID_START            "cmpIdStart"            // 起始组件ID
#define CONN_PORT_ID_END             "portIdEnd"             // 结束端口ID
#define CONN_NET_NAME                "netName"               // 网络名称
#define CONN_CMP_ID_END              "cmpIdEnd"              // 结束组件ID
#define CONN_ID                      "id"                    // 连接ID
#define CONN_PORT_ID_START           "portIdStart"           // 起始端口ID

// ============================================================================
// 组件对象字段宏定义 (cmpSet)
// ============================================================================

// 组件基本信息字段
#define CMP_SUB_SYSTEM               "subSystem"             // 子系统
#define CMP_INSTANCE_NAME            "instanceName"          // 实例名称
#define CMP_ADS_LIB                  "ADSLib"                // ADS库
#define CMP_CATEGORY                 "cmpCategory"           // 组件分类
#define CMP_ID                       "cmpId"                 // 组件ID
#define CMP_ICON                     "icon"                  // 图标(本地)
#define CMP_IS_INSTANCE_NAME_SHOW    "isInstanceNameShow"    // 是否显示实例名
#define CMP_SCALE                    "scale"                 // 缩放比例
#define CMP_IS_SUB_SYSTEM            "isSubSystem"           // 是否是子系统
#define CMP_OBJECT_TYPE              "objectType"            // 对象类型
#define CMP_LABEL_POSITION_STATE     "labelPositionState"    // 标签位置状态
#define CMP_ICON_OBJECT              "iconObject"            // 图标对象
#define CMP_SIZE                     "size"                  // 组件尺寸
#define CMP_PORT                     "port"                  // 端口数组
#define CMP_POS                      "pos"                   // 位置坐标
#define CMP_IS_CMP_TYPE_SHOW         "isCmpTypeShow"         // 是否显示组件类型
#define CMP_ICON_WEB                 "iconWeb"               // 图标(Web)
#define CMP_TYPE                     "cmpType"               // 组件类型
#define CMP_ATTRIBUTE                "attribute"             // 属性数组
#define CMP_IS_PORTEDIT              "isPortedit"            // 是否可编辑端口
#define CMP_PORT_ID_MAP              "portIdMap"             // 端口ID映射

// ============================================================================
// 端口对象字段宏定义 (port)
// ============================================================================

// 端口字段
#define PORT_ROTATE                  "rotate"                // 旋转角度
#define PORT_PUT_TYPE                "putType"               // 端口类型(in/out)
#define PORT_POS                     "pos"                   // 端口位置
#define PORT_NAME_DISPLAY            "nameDisplay"           // 名称显示设置
#define PORT_DATA_TYPE               "dataType"              // 数据类型
#define PORT_CMP_ID                  "cmpId"                 // 所属组件ID
#define PORT_NAME                    "name"                  // 端口名称
#define PORT_NUM_IN_CMP              "numInCmp"              // 在组件中的编号
#define PORT_ID                      "id"                    // 端口ID
#define PORT_LABEL                   "label"                 // 端口标签
#define PORT_LINE_LENGTH             "lineLength"            // 连接线长度

// ============================================================================
// 属性对象字段宏定义 (attribute)
// ============================================================================

// 通用属性字段
#define ATTR_UNIT_TYPE               "unitType"              // 单位类型
#define ATTR_UNIT                    "unit"                  // 单位
#define ATTR_VAL_DEFAULT             "valDefault"            // 默认值
#define ATTR_DATA_TYPE               "dataType"              // 数据类型
#define ATTR_NAME                    "name"                  // 属性名称
#define ATTR_HIDE_CONDITION          "HideCondition"         // 隐藏条件
#define ATTR_IS_DISP                 "isDisp"                // 是否显示
#define ATTR_VALUE                   "value"                 // 属性值
#define ATTR_DESC                    "desc"                  // 描述


#define NET_GROUND                    "0"                  // 描述

// ============================================================================
// 子系统相关字段宏定义
// ============================================================================
#define CHILD_JSON                "childJson"                 // 子系统JSON数据
#define CHILD_MODEL_NAME          "childModelName"           // 子系统模型名称
#define CHILD_TOPO_ID             "childTopoId"              // 子系统拓扑ID

// ============================================================================
// 公共变量字段宏定义 (vars)
// ============================================================================
#define VARS                      "vars"                     // 公共变量数组
#define VAR_ID                    "id"                       // 变量ID
#define VAR_NAME                  "name"                     // 变量名称
#define VAR_DATA_TYPE             "dataType"                 // 变量数据类型
#define VAR_DEFAULT_VALUE         "defaultValue"             // 默认值
#define VAR_UNIT                  "unit"                     // 单位
#define VAR_UNIT_TYPE             "unitType"                 // 单位类型
#define VAR_ENUMS                 "enums"                    // 枚举值
#define VAR_CONSTRAINT            "constraint"               // 约束条件
#define VAR_DISP                  "disp"                     // 是否显示
#define VAR_DESC                  "desc"                     // 描述

// ============================================================================
// 组件对象字段宏定义 (cmp_type)
// ============================================================================

#define CMP_TYPE_GROUND                     "Ground"
#define CMP_TYPE_VAC                        "V_AC"
#define CMP_TYPE_VDC                        "V_DC"
#define CMP_TYPE_V_1TONE                     "V_1Tone"
#define CMP_TYPE_V_NTONE                     "V_nTone"
#define CMP_TYPE_Term                       "Term"
#define CMP_TYPE_R                          "R"
#define CMP_TYPE_C                          "C"
#define CMP_TYPE_DIODE                      "Diode"
#define CMP_TYPE_Amplifier2                 "Amplifier2"
#define CMP_TYPE_Amplifier                  "Amplifier"
#define CMP_TYPE_bjt                        "bjt"

#define CMP_TYPE_Attenuator                        "Attenuator"
#define CMP_TYPE_HPF_Butterworth                        "HPF_Butterworth"
#define CMP_TYPE_HPF_Chebyshev                        "HPF_Chebyshev"
#define CMP_TYPE_inductor                        "inductor"
#define CMP_TYPE_LPF_Butterworth                        "LPF_Butterworth"
#define CMP_TYPE_LPF_Chebyshev                        "LPF_Chebyshev"
#define CMP_TYPE_PwrSplit2                        "PwrSplit2"
#define CMP_TYPE_PwrSplit3                        "PwrSplit3"
#define CMP_TYPE_TwoPort                        "TwoPort"
//#define CMP_TYPE_VDC                        "VDC"

#define DATA_TYPE_Array                        "Array"

// 子系统
#define CMP_TYPE_subSystem "subSystem"
#define CMP_TYPE_inPort "inPort"
#define CMP_TYPE_outPort "outPort"

// 信号源

#define CMP_TYPE_Const "Const"
#define CMP_TYPE_ConstCx "ConstCx"
#define CMP_TYPE_GaussianNoiseGen "GaussianNoiseGen"
#define CMP_TYPE_RADAR_CW "RADAR_CW"
#define CMP_TYPE_RADAR_PULSE "RADAR_PULSE"

// 处理器
#define CMP_TYPE_Abs_M "Abs_M"
#define CMP_TYPE_AdaptLinQuant "AdaptLinQuant"
#define CMP_TYPE_Add "Add"
#define CMP_TYPE_AddCx "AddCx"
#define CMP_TYPE_AddEnv "AddEnv"
#define CMP_TYPE_AddEnv_M "AddEnv_M"
#define CMP_TYPE_AddGuard "AddGuard"
#define CMP_TYPE_AddInt "AddInt"
#define CMP_TYPE_AsyncCommutator "AsyncCommutator"
#define CMP_TYPE_AsyncCommutatorCx "AsyncCommutatorCx"
#define CMP_TYPE_AsyncCommutatorEnv "AsyncCommutatorEnv"
#define CMP_TYPE_AsyncCommutatorInt "AsyncCommutatorInt"
#define CMP_TYPE_AsyncDistributor "AsyncDistributor"
#define CMP_TYPE_AsyncDistributorCx "AsyncDistributorCx"
#define CMP_TYPE_AsyncDistributorEnv "AsyncDistributorEnv"
#define CMP_TYPE_AsyncDistributorInt "AsyncDistributorInt"
#define CMP_TYPE_AutoCorr "AutoCorr"
#define CMP_TYPE_Average "Average"
#define CMP_TYPE_AverageCx "AverageCx"
#define CMP_TYPE_AverageCxWOffset "AverageCxWOffset"
#define CMP_TYPE_AvgSqrErr_M "AvgSqrErr_M"

#define CMP_TYPE_BCH_Decoder "BCH_Decoder"
#define CMP_TYPE_BCH_Encoder "BCH_Encoder"
#define CMP_TYPE_Biquad "Biquad"
#define CMP_TYPE_BiquadCascade "BiquadCascade"
#define CMP_TYPE_BitDeformatter "BitDeformatter"
#define CMP_TYPE_BitFormatter "BitFormatter"
#define CMP_TYPE_Bits "Bits"
#define CMP_TYPE_BitShiftRegister "BitShiftRegister"
#define CMP_TYPE_BitsToInt "BitsToInt"
#define CMP_TYPE_BlockAllPole "BlockAllPole"
#define CMP_TYPE_BPF_Butterworth "BPF_Butterworth"
#define CMP_TYPE_BPF_ChebyshevI "BPF_ChebyshevI"
#define CMP_TYPE_BPF_ChebyshevII "BPF_ChebyshevII"
#define CMP_TYPE_BSF_Butterworth "BSF_Butterworth"
#define CMP_TYPE_BSF_ChebyshevI "BSF_ChebyshevI"
#define CMP_TYPE_BSF_ChebyshevII "BSF_ChebyshevII"

#define CMP_TYPE_ChirpGen "ChirpGen"
#define CMP_TYPE_Chop "Chop"
#define CMP_TYPE_ChopCx "ChopCx"
#define CMP_TYPE_ChopInt "ChopInt"
#define CMP_TYPE_ChopVarOffset "ChopVarOffset"
#define CMP_TYPE_ChopVarOffsetCx "ChopVarOffsetCx"
#define CMP_TYPE_ChopVarOffsetInt "ChopVarOffsetInt"
#define CMP_TYPE_CoderRs "CoderRs"


#define CMP_TYPE_Gain "Gain"
#define CMP_TYPE_Math "Math"
#define CMP_TYPE_RectToCx "RectToCx"
#define CMP_TYPE_Sub "Sub"

// 终端块
#define CMP_TYPE_Sink "Sink"
#define CMP_TYPE_SinkCx "SinkCx"
#define CMP_TYPE_SinkEnv "SinkEnv"


#endif // JSONLINKDEFINE_H
