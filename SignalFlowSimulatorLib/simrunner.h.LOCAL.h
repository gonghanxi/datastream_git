#ifndef SIMRUNNER_H
#define SIMRUNNER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <string>
#include "Block.h"
#include "ISimRunner.h"
#include "LogExport.h"
#include "connection.h"
#include "algorithmmanager.h"

using namespace SystemVueModelBuilder;

class SimRunner:public ISimRunner
{
public:
    SimRunner(QStringList linkFiles, QString outPutPath);
//    SimRunner(const std::string& projectFile);
    ~SimRunner();
    bool start();
    bool run();
    void setWriter(ILogWriter* write);
    void setSimCfg(SimCfgData* data);
    bool portPutTypeCheck(const QString& putTypeStart,const QString& putTypeEnd);
    bool portDataTypeCheck(PortMsg::PortDataType dataTypeStart,PortMsg::PortDataType dataTypeEnd);

private:
    //json文件解析
    bool AnalysisFiles();
    //处理所有的block
    void ManageAllBlocks();
    //初始化实例
    bool InitializeBlocks();
    //处理连接关系
    bool ManageConnection();
    //递归读取blockInfo（无限嵌套子链路）
    void recursiveReadBlock(const QVector<BlockInfo> &blocksInfo, QVector<Block *> &blocks);
    //递归建立连接关系（无限嵌套子链路）
//    bool recursiveConnection(const QString& linkKey,const QVector<Connection>& connections);
    bool recursiveConnection(const BlockInfo& topBlockInfo,const PortMsg& topPortMsg,const QString& linkKey);
    bool dfsTraverseLink(const BlockInfo parentBizBlock, const PortMsg parentContainerPort, const QString &linkKey);
    //调度
    bool RunBlocks();
    //调度算法
    bool SimpleScheduler(const QString& linkKey, QVector<Block *> blocks);
    int GeneralWork(Block* currentBlock);
    //校验是否有收集器
    bool dataCollectionCheck();
    static QJsonDocument readJsonFile(const QString& filePath);
    static QMap<QString,QString> readModelConfigFiles(const QString& modelDir);
    static void traverseDirectory(const QString &path,QMap<QString, QString> &fileNames);
    QStringList m_linkFiles;
    QString m_outPutPath;
    ILogWriter* mWrite=NULL;
    QList<QString> mSortedModels;
    SimCfgData* mSimCfgData=NULL;
    BlockInfo getFinalBizBlock(const QString &linkKey);
};

extern "C" __declspec(dllexport) ISimRunner* createSimRunner(QStringList linkFiles, QString outPutPath);
#endif // SIMRUNNER_H
