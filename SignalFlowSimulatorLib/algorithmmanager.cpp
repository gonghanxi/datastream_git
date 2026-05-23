#include "algorithmmanager.h"

#include <QApplication>
#include <QDir>
#include "../Common/LogExport.h"

AlgorithmManager* AlgorithmManager::instance=NULL;
AlgorithmManager *AlgorithmManager::createInstance()
{
    if(instance==NULL)
    {
        instance=new AlgorithmManager;
    }
    return instance;
}

Block *AlgorithmManager::getAlgorithm(const QString& appPath, const QString& typeName,const QString& instanceName)
{
    // 如果已经加载过该类型的库，直接使用
    if (!libMap.contains(typeName)) {
        // 构建动态库路径
        qDebug() << "path" << appPath;
        QString fileName;
#ifdef _WIN32
        fileName = QString("%1/models/%2.dll").arg(appPath).arg(typeName);
#else
        fileName = QString("%1/models/lib%2.so").arg(appPath).arg(typeName);
        // 如果libxxx.so不存在，尝试xxx.so
        if (!QFile::exists(fileName)) {
            fileName = QString("%1/models/%2.so").arg(appPath).arg(typeName);
        }
#endif
        libMap[typeName]=new LibraryHelper(fileName);
    }

    // 创建实例
    Block* block = libMap[typeName]->create();
    if (block) {
        block->setInstanceName(instanceName.toStdString());
    }

    return block;
}

/*
Block *AlgorithmManager::getAlgorithm(QString typeName, QString instanceName, QString linkKey)
{
    if(!libMap.contains(typeName))
    {
        qDebug() << "getAlgorithm:" << typeName;
//        QString fileName=QString("%1/models/%2.dll").arg(qApp->applicationDirPath()).arg(typeName+"_DLL");
//        QString path="D:/work_grxw/code/GWDataFlowSimulator";
        QString path=QCoreApplication::applicationDirPath();
        QString fileName=QString("%1/models/%2.dll").arg(path).arg(typeName);
//        QString fileName=QDir::cleanPath("%1/models/%2.dll").arg(path).arg(typeName+"_DLL");
        //一种类型的模型动态库只加载一次
        libMap[typeName]=new LibraryHelper(fileName);
    }

    QString mapKey=linkKey+"_"+instanceName;

    if(!algorithmMap.contains(mapKey))
    {
        Block *mapValue=libMap[typeName]->create();
        if(mapValue)
        {
            algorithmMap[mapKey]=mapValue;
            mapValue->setInstanceName(instanceName.toStdString());
        }
    }
    return algorithmMap[mapKey];
}
*/

//Block *AlgorithmManager::getAlgorithmOnlyByName(QString instanceName)
//{
//    if(algorithmMap.contains(instanceName))
//    {
//        return algorithmMap[instanceName];
//    }else
//    {
//        return NULL;
//    }
//}

//Block *AlgorithmManager::getAlgorithmById(int id)
//{
//    for(auto e :algorithmMap)
//    {
//        if(e->getId()==id)
//        {
//            return e;
//        }
//    }
//    return NULL;
//}

QMap<QString, QVector<Block *> > AlgorithmManager::getRunBlocks()
{
    return mRunBlocksMap;
}

QMap<QString, QVector<BlockInfo> > AlgorithmManager::getBlocksInfo()
{
    return mBlocksInfoMap;
}

QMap<QString, SimuParameter> AlgorithmManager::getSimuParameters()
{
    return mSimuParameters;
}

QMap<QString, QVector<Connection> > AlgorithmManager::getConnection()
{
    return mConnectionsMap;
}

AlgorithmManager::SchedulerType AlgorithmManager::getSchedulerType()
{
    return m_schedulertype;
}

void AlgorithmManager::addRunBlocks(const QString &linkKey, const QVector<Block *> &blocks)
{
    mRunBlocksMap[linkKey]=blocks;
}

//QVector<Block *> AlgorithmManager::getAlgorithmRunList()
//{
//    //todo
//    QVector<Block *> res;
////    res.append(algorithmMap["PulseCx0"]);
////    res.append(algorithmMap["AMP0"]);
////    res.append(algorithmMap["Plot0"]);
//    return res;
//}

//void AlgorithmManager::addBlocks(const QString &linkKey, Block* block)
//{
//    mBlocksMap[linkKey].push_back(block);
//}

void AlgorithmManager::addBlocksInfo(const QString &linkKey, BlockInfo blockInfo)
{
    mBlocksInfoMap[linkKey].push_back(blockInfo);
}

void AlgorithmManager::addSimuParameters(const QString &linkKey, SimuParameter simu)
{
    mSimuParameters[linkKey]=simu;
}

void AlgorithmManager::addConnection(const QString &linkKey, const Connection &connection)
{
    mConnectionsMap[linkKey].push_back(connection);
}

void AlgorithmManager::setSchedulerType(AlgorithmManager::SchedulerType type)
{
    m_schedulertype = type;
}

void AlgorithmManager::clear()
{
//    for(auto e:algorithmMap)
//    {
//        delete e;
//    }
//    algorithmMap.clear();
    for(auto blocks:mBlocksInfoMap)
    {
        for(auto e:blocks)
        {
            delete e.block;
        }
    }
    mBlocksInfoMap.clear();
}

AlgorithmManager::AlgorithmManager()
{

}

AlgorithmManager::~AlgorithmManager()
{
    for(auto e:libMap)
    {
        delete e;
    }
    clear();
}

//QString AlgorithmManager::genNodeName()
//{
//    static int nodeId=0;
//    return QString("N_%d").arg(nodeId);
//}
