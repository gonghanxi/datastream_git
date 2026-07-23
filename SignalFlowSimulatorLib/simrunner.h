#ifndef SIMRUNNER_H
#define SIMRUNNER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <string>
#include <QMutex>
#include <QWaitCondition>

#include "../Common/ILogWriter.h"
#include "../Common/LogExport.h"
#include "../Common/ISimRunner.h"

#include "connection.h"
#include "algorithmmanager.h"
#include "Block.h"
#include "FMUModelInfo.h"

#include "DataStreamVerification.h"
#include "ModelCompatCheck.h"
#include "signalflowlinksort.h"
#include "ShortOpenProcessor.h"

#include "SimEngineController.h"

#include "ReadyQueueScheduler.h"
#include "SimpleScheduler.h"
#include "TimeDrivenScheduler.h"
#include "EventDrivenScheduler.h"

#ifdef _WIN32
    #ifdef SignalFlowSimulatorLib_EXPORTS
        #define SIMRUNNER_EXPORT __declspec(dllexport)
    #else
        #define SIMRUNNER_EXPORT __declspec(dllimport)
    #endif
#else
    #ifdef SignalFlowSimulatorLib_EXPORTS
        #define SIMRUNNER_EXPORT __attribute__((visibility("default")))
    #else
        #define SIMRUNNER_EXPORT
    #endif
#endif

using namespace SystemVueModelBuilder;

class SimRunner:public ISimRunner
{
public:
//    SimRunner(QString appPath, QStringList linkFiles, QString outPutPath);
    SimRunner(const char* appPath,
               const char** linkFiles,
               int fileCount,
               const char* outPutPath);
//    SimRunner(const std::string& projectFile);
    ~SimRunner();
    bool start() override;
    bool run() override;
    bool hasSubSystems() const;
    void setWriter(ILogWriter* write) override;
    void setSimCfg(SimCfgData* data) override;
    bool portPutTypeCheck(const QString& putTypeStart,const QString& putTypeEnd);
    bool portDataTypeCheck(PortMsg::PortDataType dataTypeStart,PortMsg::PortDataType dataTypeEnd);

    // 添加获取仿真参数的方法
    SimuParameter getSimulationParameters() const override;

    // 获取高级步长信息（AdvancedSimuParam 解析结果）
    AdvancedStepInfo getAdvancedStepInfo() const override { return m_advancedStepInfo; }

    // ========== 暂停/继续/停止控制接口 ==========
    void pause() override;         // 暂停仿真
    void resume() override;        // 继续仿真
    void requestStop() override;   // 请求停止仿真
    bool isPaused() const;         // 查询暂停状态

    // 获取暂停控制指针，供外部直接操作原子标志
    QAtomicInt* getPausedPtr() override { return &m_paused; }
    QAtomicInt* getStopRequestedPtr() override { return &m_stopRequested; }
    QMutex* getPauseMutexPtr() override { return &m_pauseMutex; }
    QWaitCondition* getPauseCondPtr() override { return &m_pauseCond; }

    //设置是否为时间驱动

    //DDS服务
    void setStopSignalPath(const QString &stopFilePath) override;
    std::map<std::string, std::string> GetSinksOutPutPaths() override;
    void SetLinkFiles(const char **linkFiles, int FileCount) override;
    bool Initialize() override;
    bool Setup() override;
    bool Stop() override;
    int GetModelStatus() override;
    bool OnCurrStepChanged(int curStep) override;
    void SetLogCallback(LogCallback callback) override;
    bool OnEventStepChanged(const QString &bits) override;


private:
    //json文件解析
    bool AnalysisFiles();
    //处理所有的block
    void ManageAllBlocks();
    //初始化实例
    bool InitializeBlocks();
    //处理连接关系
    bool ManageConnection();
    bool validateLinkAndSubLinks(const QString& linkKey);//递归校验链路及其所有子链路
    bool validateShortCircuitedSourcesAndSinks();//短路模型校验
    //Setup所有的Block
    bool SetupBlocks();
    //递归读取blockInfo（无限嵌套子链路）
    void recursiveReadBlock(QVector<BlockInfo> &blocksInfo, QVector<Block *> &blocks);
    //递归建立连接关系（无限嵌套子链路）
    bool dfsTraverseLink(const BlockInfo& upstreamBlock, const PortMsg& upstreamPort, const PortMsg& parentPort, const QString& linkKey);
    //递归建立连接关系（无限嵌套子链路）
    bool dfsTraverseLink(const BlockInfo& upstreamBlock, const PortMsg& upstreamPort, const BlockInfo& parentBlock, const PortMsg& parentPort, const QString& linkKey);
    bool isSubSystemEmpty(const QString& subLinkKey);//判断子系统是否为空
    bool checkSubLinkTopProtId(const QString& childTopoId, const PortMsg& entryPort, const QString& portType = "inPort");//校验子链路入口端口的topProtId是否匹配
    //调度
    bool RunBlocks();
    bool NewScheduler();
    bool OldScheduler();
    bool TimeScheduler();
    bool EventScheduler();

    QString m_appPath;//程序所在路径
    QStringList m_linkFiles;//链路文件名
    QString m_outPutPath;//输出路径
    QString m_currentMainLinkKey;//主链路

    ILogWriter* mWrite=NULL;
    QList<QString> mSortedModels;
    SimCfgData* mSimCfgData=NULL;
    QVector<QString> mConnections;
    QVector<QString> mNullParents;
    //矩阵校验指针
    std::shared_ptr<DataStreamVerification> m_verificationSystem;
    //仿真器参数缓存
    QMap<QString, SimuParameter> m_simuParamsCache;
    // 高级步长参数（从 AdvancedSimuParam 解析）
    AdvancedStepInfo m_advancedStepInfo;
    // 拓扑排序器
    SignalFlowLinkSort m_topologySorter;
    // 存储短路且为信号源或数据收集器的模型
    QVector<ShortCircuitedModel> m_shortCircuitedSourcesAndSinks;

    //用户id
    QString m_UserId;

    // ========== 暂停/继续/停止控制成员 ==========
    QAtomicInt m_paused;           // 暂停标志: 0=运行, 1=暂停
    QAtomicInt m_stopRequested;    // 停止请求标志: 0=运行, 1=请求停止
    QMutex m_pauseMutex;           // 暂停互斥锁
    QWaitCondition m_pauseCond;   // 暂停条件变量

    // =========== DDS 服务成员 ===========
    SimEngineController m_dds;

    //调度器（数据流、时间、事件）
    enum class ActiveScheduler { NONE, DATA_STREAM, TIME_DRIVEN, EVENT_DRIVEN };//调度器类型标识
    ActiveScheduler m_activeScheduler;
    SimpleScheduler m_simpleScheduler;
    TimeDrivenScheduler m_timeDrivenScheduler;
    EventDrivenScheduler m_eventDrivenScheduler;
};

#ifdef __cplusplus
extern "C" {
#endif

SIMRUNNER_EXPORT ISimRunner* createSimRunner(
    const char* appPath,
    const char** linkFiles,
    int fileCount,
    const char* outPutPath);

#ifdef __cplusplus
}
#endif

#endif // SIMRUNNER_H
