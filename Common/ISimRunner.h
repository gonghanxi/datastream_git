#ifndef ISIMRUNNER_H
#define ISIMRUNNER_H

#include "ILogWriter.h"
#include "SimuParameter.h"
#include "simcfgdata.h"
#include <map>
#include <string>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>

class ISimRunner{
public:

    virtual bool start()=0;
    virtual bool run()=0;


    virtual void setWriter(ILogWriter* write)=0;
    //virtual Port* getPort(std::string instanceName,std::string portId)=0;
    virtual void setSimCfg(SimCfgData* data)=0;
    virtual SystemVueModelBuilder::SimuParameter getSimulationParameters() const = 0;
    virtual ~ISimRunner(){}

    virtual void setStopSignalPath(const QString& stopFilePath) = 0;
    virtual std::map<std::string, std::string> GetSinksOutPutPaths() = 0;
    virtual void SetLinkFiles(const char** linkFiles, int FileCount) = 0;
    //DDS服务 - 控制指令: 初始化
    virtual bool Initialize() = 0;
    //DDS服务 - 控制指令: 开始
    virtual bool Setup() = 0;
    //DDS服务 - 控制指令: 停止
    virtual bool Stop() = 0;

    //DDS服务 - 心跳: 获取状态
    virtual int GetModelStatus() = 0;

    //DDS服务 - 周期: 当前节拍
    virtual bool OnCurrStepChanged(int curStep) = 0;

    //DDS服务 - 周期: 日志上传
    using LogCallback = std::function<void(const char* level, const char* message, const char* timestamp)>;
    virtual void SetLogCallback(LogCallback callback) = 0;

    //    //DDS服务 - 控制指令: 控制执行发布
    //    virtual int OnSimulationControlChanged(const char *modelVer,const char *modelName,int ctrlType) = 0;

    //    //DDS服务 - 周期: 当前节拍
    //    virtual bool OnCurrStepChanged(const char *modelVer,const char *modelName, int curStep, int errorHandle) = 0;

    //    //DDS服务 - 周期: 上游输入数据
    //    virtual int OnRecvData(const char *modelVer,const char *modelName, void *pData, int size) = 0;

    //    //DDS服务 - 周期: 事件节拍
    virtual bool OnEventStepChanged(const QString& bits) = 0;

    //    //DDS服务 - 周期: 上游事件数据
    //    virtual bool OnEventDataRecv(const char *modelVer,const char *modelName, const char *eventID, void *pData, int size) = 0;

    //    //DDS服务 - 周期: 时间同步
    //    virtual bool OnCurrTimeChanged(const char *modelVer,const char *modelName, int time) = 0;

    //    //DDS服务 - 周期: 可变参数配置
    //    virtual int OnVariableParameterChanged(const char *modelVer,const char *modelName, void *data, std::list<int> offsetList) = 0;
    //    // ============= DDS服务接口 =============
    //暂停
    virtual void pause() = 0;
    //复位
    virtual void resume() = 0;
    //停止
    virtual void requestStop() = 0;

    // 获取暂停控制指针，供外部直接操作原子标志
    virtual QAtomicInt* getPausedPtr() = 0;
    virtual QAtomicInt* getStopRequestedPtr() = 0;
    virtual QMutex* getPauseMutexPtr() = 0;
    virtual QWaitCondition* getPauseCondPtr() = 0;
};
//typedef ISimRunner* (*CreateSimFunction)(QString, QStringList, QString);
// 使用const char* (UTF-8编码) 作为接口
typedef ISimRunner* (*CreateSimFunction)(const char* appPath,
                                         const char** linkFiles,
                                         int fileCount,
                                         const char* outPutPath);

//typedef ISimRunner* (*CreateSimFunction)(std::string);
#endif // ISIMRUNNER_H
