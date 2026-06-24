#ifndef ALGORITHMHELPER_H
#define ALGORITHMHELPER_H

#include <qlibrary.h>
#include <QString>
#include <QStringList>
#include <QMap>
#include "Block.h"
#include "connection.h"
#include "libraryhelper.h"
using namespace SystemVueModelBuilder;

//端口信息
//struct PortMsg
//{
//    enum PortDataType {
//        INT,
//        COMPLEX,
//        ANYTYPE,
//        ENVELOPE,
//        REAL,
//        FIXEDPOINT,
//        VARIANT,
//        MULTIPLE_INT,
//        MULTIPLE_COMPLEX,
//        MULTIPLE_ANYTYPE,
//        MULTIPLE_ENVELOPE,
//        MULTIPLE_REAL,
//        MULTIPLE_FIXEDPOINT,
//        MULTIPLE_VARIANT,
//        INT_MATRIX,
//        COMPLEX_MATRIX,
//        ANYTYPE_MATRIX,
//        ENVELOPE_MATRIX,
//        REAL_MATRIX,
//        FIXEDPOINT_MATRIX,
//        VARIANT_MATRIX,
//        MULTIPLE_INT_MATRIX,
//        MULTIPLE_COMPLEX_MATRIX,
//        MULTIPLE_ANYTYPE_MATRIX,
//        MULTIPLE_ENVELOPE_MATRIX,
//        MULTIPLE_REAL_MATRIX,
//        MULTIPLE_FIXEDPOINT_MATRIX,
//        MULTIPLE_VARIANT_MATRIX
//    };

//    // 重新排序：QString (指针大小) > int > enum > bool
//    QString putType;      // 8字节 (64位系统)
//    QString name;         // 8字节
//    PortDataType dataType;// 4字节 (enum通常为int大小)
//    int id;               // 4字节
//    int topProtId;        // 4字节
//    bool isOptional;      // 1字节 + 3字节填充
//    unsigned int portRate;
//    // 现在大小：8+8+4+4+4+1+3(填充)=32字节
//};

//block信息
struct BlockInfo
{
    int cmpId;
    bool isSubSystem;
    QString cmpType;
    QString instanceName;
    QString childTopoId;
    Block* block;
    //端口信息 Key:portId Value:Port信息
    QMap<int,PortMsg> portsMsg;
    //参数信息 key：参数名 Value:参数值 为了匹配block里的参数信息，用std::map
    std::map<std::string,Parameter> parameters;
    // 子系统路径（新增）- 例如 "Transmitter" 或 "Transmitter/SubSystem2"
    QString subsystemPath;

    // FMU相关字段
    QVector<QString> dllOrSoPaths;           // FMU动态库路径列表
    QString guid;                            // guid
    QMap<int, int> portValueReferences;      // 端口ID -> valueReference
    QMap<QString, int> paramValueReferences; // 参数名 -> valueReference
    // 标志位
    bool isFmuModel = false;                 // 是否为FMU模型
    bool isinPort = false;                   // 是否为inPort模型

    // CFunction相关字段
    bool isCFunctionModel = false;           // 是否为CFunction模型
    QString cfunctionLanguage;               // 编译语言 "c" 或 "cpp"
    QStringList cfunctionLibFilePaths;       // 库文件路径列表
    QStringList cfunctionLibFileNames;       // 库文件名列表
    QStringList cfunctionHeaderFilePaths;    // 头文件路径列表
    QStringList cfunctionHeaderFileNames;    // 头文件名列表
    QStringList cfunctionCFilePaths;         // 源文件路径列表
    QStringList cfunctionCFileNames;         // 源文件名列表
    QString cfunctionEquations;              // Equations代码
    QString cfunctionGeneratedJsonPath;      // 生成的cfunction.json绝对路径

    //短路开路
    QString cmpCondition;
    QString cmpCategory;
};

class AlgorithmManager
{
public:
    enum SchedulerType {DATA_STREAM, TIME_DRIVEN, ENVENT_DRIVEN};
    static AlgorithmManager* createInstance();
    Block* getAlgorithm(const QString& appPath, const QString& typeName,const QString& instanceName);
//    Block* getAlgorithm(const QString& typeName,const QString& instanceName,const QString& linkName);
//    Block* getAlgorithmOnlyByName(QString instanceName);
//    Block* getAlgorithmById(int id);
//    QVector<Block*> getAlgorithmRunList();
    QMap<QString, QVector<Block*>> getRunBlocks();
    QMap<QString, QVector<struct BlockInfo>> getBlocksInfo();
    QMap<QString, SimuParameter> getSimuParameters();
    QMap<QString, QVector<Connection>> getConnection();
    SchedulerType getSchedulerType();
//    void addBlocks(QString linkKey, Block* block);
    void addRunBlocks(const QString& linkKey,const QVector<Block*>& blocks);
    void addBlocksInfo(const QString& linkKey,BlockInfo blockInfo);
    void addSimuParameters(const QString& linkKey, SimuParameter simu);
    void addConnection(const QString& linkKey,const Connection& connection);
    void setSchedulerType(SchedulerType type);
    void clear();
    //Node* getNode(QString nodeName);

private:
    //block信息
    AlgorithmManager();
    ~AlgorithmManager();
    static AlgorithmManager* instance;
//    QString genNodeName();

    //存储模型动态库信息 key值：typeName Value值：LibraryHelper指针
    QMap<QString,LibraryHelper*> libMap;
    //按链路存储block实例 key值：linkKey Value值：该链路所有block实例地址
    QMap<QString, QVector<Block*>> mRunBlocksMap;
    //按链路存储block信息 key值：linkKey Value值：该链路所有block信息
    QMap<QString, QVector<struct BlockInfo>> mBlocksInfoMap;
    //仿真器参数 key值：linkKey Value值：该链路的仿真器参数
    QMap<QString, SimuParameter> mSimuParameters;
    //根据linkKey存储连接关系
    QMap<QString, QVector<Connection>> mConnectionsMap;
    //数据收集器类型
//    static inline const QStringList DataCollection={"Sink","SpectrumAnalyzer"};
    //QMap<QString,Node*> nodeMap;
    //调度器类型（数据流、时间、事件）
    SchedulerType m_schedulertype;
};

#endif // ALGORITHMHELPER_H
