#ifndef SIMRUNNER_H
#define SIMRUNNER_H
#include "signalflowlinksort.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <string>
#include <Block.h>
#include "../Common/ILogWriter.h"
#include "../Common/LogExport.h"
#include "../Common/ISimRunner.h"

using namespace SystemVueModelBuilder;

class SimRunner:public ISimRunner
{
public:


    SimRunner(QStringList linkFiles, QString outPutPath);
//    SimRunner(const std::string& projectFile);
    ~SimRunner();
    bool start();
    bool run();
    //调度算法
    void SimpleScheduler(const QString& linkKey, QVector<Block *> blocks);
    int GeneralWork(Block* currentBlock);

    void setWriter(ILogWriter* write);
    //Port* getPort(std::string instanceName,std::string portId);
    void setSimCfg(SimCfgData* data);
//    QVector<Block *> getBlock(const QString& simuName) const
//    {
//        return mBlocks[simuName];
//    }
    bool portDataTypeCheck(PortMsg::PortDataType portStart,PortMsg::PortDataType portEnd);


private:
    bool Init();
    static QJsonDocument readJsonFile(const QString& filePath);
    static QMap<QString,QString> readModelConfigFiles(const QString& modelDir);
    static void traverseDirectory(const QString &path,QMap<QString, QString> &fileNames);
    SignalFlowLinkSort sortHelper;
//    std::string m_projectFile;
//    std::string m_modelFolder;
//    std::string m_linkFiles;
//    QVector<QString> m_linkFiles;
    QStringList m_linkFiles;
    QString m_outPutPath;
    ILogWriter* mWrite=NULL;
    QList<QString> mSortedModels;
    SimCfgData* mSimCfgData=NULL;
//    std::vector<Block *> mBlocks;
//    QMap<QString, QVector<Block*>> mBlocks;
//    QMap<QString, Simu> mSimuParameters;  //key

};
//extern "C" __declspec(dllexport) ISimRunner* createSimRunner(std::string linkFiles, std::string outPutPath);
//extern "C" __declspec(dllexport) ISimRunner* createSimRunner(std::string projectFile);
extern "C" __declspec(dllexport) ISimRunner* createSimRunner(QStringList linkFiles, QString outPutPath);
#endif // SIMRUNNER_H
